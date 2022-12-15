#pragma once

#include <algorithm>
#include <queue>
#include <functional>
#include <string>

// Объявление лепестка наперёд.
template<typename T, uint16_t N>
class NLeaf;

// Объявление дерева наперёд.
template<typename T, uint16_t N>
using NTree = NLeaf<T, N>;

// Данные, используемые для генерации и десериализации лепестка.
template<typename T, uint16_t N>
struct leaf_generation_data_t
{
	// Указатель на место, куда должен будет поместиться указатель на сгенерированный лепесток в будущем.
	NLeaf<T, N>** output;

	// Родитель лепестка, который необходимо сгенерировать.
	NLeaf<T, N>* parent;

	// Индекс лепестка в массиве потомков.
	uint16_t childIndex;
};

// Имплементация лепестка (и дерева).
template<typename T, uint16_t N>
class NLeaf
{
public:
	/*
		Этот callback используется в итерации по дереву. Его задаёт программист, чтобы
		указать функционал, который должен исполнится на каждый лепесток дерева.

		Если эта лямбда вернёт true, то итерация сразу же прекратится, и цикл не перейдёт к
		следующему потомку. Таким образом программист может в любое время вызвать своеобразный "break".

		"continue" же будет возвращением false, так как при возвращении false лямбда не продолжает исполнение
		и метод итерации просто переходит на следующего потомка.
	*/
	using walk_callback_t = std::function<bool(NLeaf<T, N>*)>;

	/*
		Эта лямбда используется в десериализации дерева. Её задача - превратить строковое
		значение лепестка в исходное состояние. Например, если есть строка "123", то эта лямбда
		должна превратить её в int значение 123, учитывая, что T лепестка равняется int.
	*/
	using deserializer_t = std::function<T(const std::string&)>;
private:
	// Значение лепестка.
	T mValue;

	// Глубина лепестка.
	uint16_t mDepth;

	// Индекс лепестка в массиве потомков.
	uint16_t mChildIndex;

	// Количество детей данного лепестка.
	uint16_t mChildrenAmount;

	// Потомки лепестка.
	NLeaf<T, N>* mChildren[N];
public:
	// Стандартный конструктор лепестка.
	NLeaf()
	{
		mValue = T();

		mDepth = 0;
		mChildIndex = 0;

		mChildrenAmount = 0;
		memset(mChildren, 0, sizeof(mChildren));
	}

	// Конструктор лепестка, задающий изначальное значение.
	NLeaf(T value)
	{
		mValue = value;

		mDepth = 0;
		mChildIndex = 0;

		mChildrenAmount = 0;
		memset(mChildren, 0, sizeof(mChildren));
	}

	// Деструктор лепестка, уничтожающий всех потомков в цикле. Метод Walk описывается чуть ниже.
	~NLeaf()
	{
		// Проходимся по всем потомкам и удаляем их, не включая себя.
		Walk([](NLeaf<T, N>* leaf) -> bool {
			delete leaf;

			return false;
		}, false);
	}
public:
	// Получение размера всего дерева в байтах.
	size_t GetByteSize()
	{
		size_t result = 0;

		// Проходимся по всем потомкам и добавляем их размер к сумме, включая себя.
		Walk([&](NLeaf<T, N>* leaf) -> bool {
			result += sizeof(*leaf);

			return false;
		});

		return result;
	}
public:
	/*
		Этот метод использует способ очереди для итерации по всем
		потомкам текущего лепестка (или корня дерева, так как он тоже является лепестком).

		Callback "walker" вызывается на каждого потомка этого лепестка.

		Если флаг includeSelf установлен в false, то лямбда walker не будет вызвана
		на корень, то есть на лепесток, который вызвал метод Walk. Это нужно, чтобы пройтись
		только по потомкам лепестка, не включая сам лепесток.
	*/
	void Walk(walk_callback_t walker, bool includeSelf = true)
	{
		// Очередь лепестков для итерации.
		std::queue<NLeaf<T, N>*> collected = {};

		/*
			Если надо добавить текущий лепесток, то добавляем this в очередь.
			Иначе добавляем только потомков mLeft и mRight.
		*/
		if (includeSelf)
		{
			collected.push(this);
		}
		else
		{
			for (uint16_t c = 0; c < mChildrenAmount; c++)
			{
				collected.push(mChildren[c]);
			}
		}

		// Пока в очереди есть лепестки...
		while (collected.size() > 0)
		{
			// Получаем первый на очереди лепесток.
			NLeaf<T, N>* leaf = collected.front();
			collected.pop();

			// Добавляем всех потомков полученного лепестка в очередь, если они есть.

			for (uint16_t c = 0; c < leaf->mChildrenAmount; c++)
			{
				collected.push(leaf->mChildren[c]);
			}

			// Вызываем переданную в Walk лямбду и передаём туда полученный лепесток. Ожидаем, чтобы она вернула bool.
			bool shouldStop = walker(leaf);

			// Если лямбда вернула true, то прекращаем проходиться по очереди.
			if (shouldStop)
			{
				break;
			}
		}
	}
public:
	/* 
		Методы установки потомков лепестка.
		При их вызове устанавливается соответсвующий индекс и глубина.
	*/

	void SetNChild(uint16_t index, NLeaf<T, N>* leaf)
	{
		mChildren[index] = leaf;

		mChildren[index]->mChildIndex = index;
		mChildren[index]->mDepth = mDepth + 1;

		mChildrenAmount++;
	}
	
	// Получение потомков соответственно.

	NLeaf<T, N>* GetNChild(uint16_t index) const
	{
		return mChildren[index];
	}

	/* 
		Получение указателей на поле потомка по индексу данного лепестка.
		Это нужно, чтобы записать сгенерированные лепестки в данные поля в будущем.
	*/

	NLeaf<T, N>** GetNChild(uint16_t index)
	{
		return &mChildren[index];
	}

	// Установка и получение значения этого лепестка.

	T GetValue()
	{
		return mValue;
	}

	void SetValue(T value)
	{
		mValue = value;
	}

	// Получение глубины этого лепестка.

	uint16_t GetDepth()
	{
		return mDepth;
	}

	uint16_t GetChildAmount()
	{
		return mChildrenAmount;
	}

	uint16_t GetChildIndex()
	{
		return mChildIndex;
	}
public:
	/*
		Этот метод просто проходится по всем потомкам, включая текущий лепесток, и находит максимальное количество ветвлений.
		
		Максимальное значение записывается по ссылке output.
		Соответствующее ему поддерево записывается по ссылке outputHolder.
	*/
	void GetMaxChildrenSubtree(int& output, NLeaf<T, N>*& outputHolder)
	{
		Walk([&](NLeaf<T, N>* leaf) -> bool {
			int amount = leaf->GetChildAmount();
			
			if (amount > output)
			{
				output = amount;
				outputHolder = leaf;
			}

			return false;
		});
	}
public:
	/*
		Метод сериализации. Приводит дерево в вид, который можно либо хранить в файле, либо вывести в консоль.

		stream - поток вывода, куда дерево будет сериализовываться. Может быть как cout, так и ofstream.

		Следующие аргументы не стоит передавать для хранения в файле, так как десериализатор их не обрабатывает:
		skipDeep - при достижении данной глубины сериализация прекратится. Может быть -1 в случае если ограничение не требуется.
		pretty - включить табуляцию.
	*/
	void Serialize(std::ostream& stream, uint16_t skipDeep = -1, bool pretty = false)
	{
		Walk([&](NLeaf<T, N>* leaf) -> bool {
			// "Красивизация" дерева.
			if (pretty)
			{
				// Максимальное количество табов - 32.
				uint16_t tabDepth = (leaf->mDepth < 32) ? leaf->mDepth : 32;

				// Дальние лепестки будут чуть дальше от левого края, чтобы их различать было легче.
				tabDepth += leaf->mChildIndex;

				// Вывод табов.
				for (uint16_t t = 0; t < tabDepth; t++)
				{
					stream << "\t";
				}

				// Вывод глубины и двоеточия.
				stream << leaf->mDepth << ": ";
			}
			
			// Вывод количество детей лепестка, разделителя, значения лепестка и перенос на следующую строку.
			stream << leaf->mChildrenAmount << ":" << leaf->mValue << std::endl;

			// Если skipDeep включен и мы его достигли по глубине, то не продолжать дальше выводить лепестки.
			if (skipDeep != -1 && leaf->mDepth > skipDeep)
			{
				stream << "..." << std::endl;

				return true;
			}

			return false;
		});
	}

	/*
		Метод десериализации (статический). Из дерева в файле создаёт дерево в коде и записывает его по указателю output.

		stream - поток ввода. может быть как cin, так и ifstream.
		valueDeserializer - десериализатор строковых значений в T данного лепестка.
	*/
	static void Deserialize(std::istream& stream, NLeaf<T, N>** output, deserializer_t valueDeserializer)
	{
		// Очередь лепестков на популяцию.
		std::queue<leaf_generation_data_t<T, N>> toPopulate = {};
		toPopulate.push({ output, nullptr, 0 });

		// Текущая строка в потоке.
		std::string curline = "";

		// Пока в потоке есть данные и пока очередь на популяцию не пустая...
		while (stream.good() && toPopulate.size() > 0)
		{
			// Получаем текущую строку и проверяем её на пустоту.
			std::getline(stream, curline);
			if (curline.size() <= 0)
			{
				continue;
			}

			// Ищем разделитель в строке с данными лепестка.
			size_t delimiterPos = curline.find(':');
			if (delimiterPos == std::string::npos)
			{
				continue;
			}

			// Извлекаем строки между разделителем.
			std::string childAmountString = curline.substr(0, delimiterPos);
			std::string valueString = curline.substr(delimiterPos + 1, curline.size() - delimiterPos);

			// Преобразовываем строку с количеством детей в количество детей для лепестка.
			uint16_t childrenAmount = valueDeserializer(childAmountString);

			// Преобразовываем строку со значением в T значение лепестка.
			T value = valueDeserializer(valueString);

			// Создаём лепесток с преобразованным значением.
			const leaf_generation_data_t<T, N>& leafData = toPopulate.front();
			(*leafData.output) = new NLeaf<T, N>(value);

			// Устанавливаем иерархию, индекс лепестка и его глубину.
			if (leafData.parent != nullptr)
			{
				leafData.parent->SetNChild(leafData.childIndex, (*leafData.output));
			}

			// Добавляем в очередь на популяцию всех потомков созданного лепестка.
			for (uint16_t c = 0; c < childrenAmount; c++)
			{
				toPopulate.push({ (*leafData.output)->GetNChild(c), (*leafData.output), c });
			}

			// Удаляем из очереди на популяцию созданный лепесток.
			toPopulate.pop();
		}
	}
};
