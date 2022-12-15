#include "profile.hpp"

#include <iostream>
#include <cstdlib>

#include <fstream>

#include "ntree.hpp"

// Генерирует N дерево. maxLeaves - максимальное количество элементов.
NTree<int, 5>* GenerateTree(int maxLeaves)
{
	// Установка сида для рандомных значений лепестков.
	srand(time(NULL));

	NTree<int, 5>* result = nullptr;

	// Очередь на генерацию.
	std::queue<leaf_generation_data_t<int, 5>> toGenerate = {};
	toGenerate.push({ &result, nullptr, 0 });

	int leavesGenerated = 0;

	// Пока есть лепестки в очереди на генерацию...
	while (toGenerate.size() > 0)
	{
		// Получить данные о запросе на генерацию.
		const leaf_generation_data_t<int, 5>& leafData = toGenerate.front();

		// Создать лепесток по этим данным со случайным значением.
		int leafValue = rand() % 255;
		(*leafData.output) = new NLeaf<int, 5>(leafValue);

		// Устанавливаем иерархию, индекс лепестка и его глубину.
		if (leafData.parent != nullptr)
		{
			leafData.parent->SetNChild(leafData.childIndex, (*leafData.output));
		}

		// Проверяем лимит лепестков.
		leavesGenerated++;
		if (leavesGenerated >= maxLeaves)
		{
			break;
		}

		int childrenAmount = 2 + (rand() % 4);

		// Добавляем в очередь на популяцию всех будущих потомков созданного лепестка.
		for (uint16_t c = 0; c < childrenAmount; c++)
		{
			toGenerate.push({ (*leafData.output)->GetNChild(c), (*leafData.output), c });
		}

		// Удаляем из очереди на генерацию данные о созданном лепесток.
		toGenerate.pop();
	}

	return result;
}

int main(int argc, const char** argv)
{
	// Открываем поток ввода для файла tree.nt
	std::ifstream input = std::ifstream("ntree.nt");

	// Поток вывода пока что не открыт, но объявлен.
	std::ofstream output;

	NTree<int, 5>* tree = nullptr;

	if (input.is_open())
	{
		// Десериализация.

		// Запускаем профилизацию памяти и времени.
		profile::StartMemoryProfiling();
		profile::StartTimeProfiling();

		// Десериализацией подгружаем дерево из потока ввода.
		tree = new NTree<int, 5>();
		NTree<int, 5>::Deserialize(input, &tree, [](const std::string& serialized) -> int {
			// Это лямбда обработки строкового значения. Тут мы просто преобразуем строковое число в int.
			return std::stoi(serialized);
		});

		// Завершаем профилизацию памяти и времени.
		profile::EndTimeProfiling();
		profile::EndMemoryProfiling();

		// Выводим информацию, полученную за время профилизации.
		std::cout << "1. Deserialization (loading from file) took " << profile::GetProfiledTime().count() << " microseconds." << std::endl;
		std::cout << "\t with " << profile::GetProfiledMemory() << " bytes of memory allocated in total" << std::endl << std::endl;

		input.close();
	}
	else
	{
		// Генерация.

		int maxLeaves = 0;

		// Запрашиваем у пользователя количество лепестков.
		std::cout << "Enter max amount of leaves: " << std::endl;
		std::cin >> maxLeaves;

		profile::StartMemoryProfiling();
		profile::StartTimeProfiling();

		// Генерируем дерево.
		tree = GenerateTree(maxLeaves);

		profile::EndTimeProfiling();
		profile::EndMemoryProfiling();

		std::cout << "1. Generation took " << profile::GetProfiledTime().count() << " microseconds." << std::endl;
		std::cout << "\t with " << profile::GetProfiledMemory() << " bytes of memory allocated in total" << std::endl << std::endl;

		// Открываем поток вывода, так как после генерации дерево нужно вывести в файл.
		output = std::ofstream("ntree.nt");
	}

	NTree<int, 5>* maxChildrenSubtree = nullptr;
	int maxChildren = 0;

	// Нахождение лепестка с максимальным количеством ветвлений.

	profile::StartMemoryProfiling();
	profile::StartTimeProfiling();

	tree->GetMaxChildrenSubtree(maxChildren, maxChildrenSubtree);

	profile::EndTimeProfiling();
	profile::EndMemoryProfiling();

	std::cout << "2. Search took " << profile::GetProfiledTime().count() << " microseconds." << std::endl;
	std::cout << "\t with " << profile::GetProfiledMemory() << " bytes of memory allocated in total" << std::endl << std::endl;

	// Если поток вывода открыт, сериализируем дерево.
	if (output.is_open())
	{
		// Сериализация.

		profile::StartMemoryProfiling();
		profile::StartTimeProfiling();

		tree->Serialize(output);

		profile::EndTimeProfiling();
		profile::EndMemoryProfiling();

		std::cout << "3. Serialization (writing to file) took " << profile::GetProfiledTime().count() << " microseconds." << std::endl;
		std::cout << "\t with " << profile::GetProfiledMemory() << " bytes of memory allocated in total" << std::endl << std::endl;

		output.close();
	}

	// Сериализируем основное дерево, его размер, а так же найденные отношения и поддеревья в поток cout.
	// Таким образом сериализованные данные выведутся в консоль.

	std::cout << tree->GetByteSize() << " bytes used by tree" << std::endl;
	std::cout << std::endl << "Tree: " << std::endl;

	tree->Serialize(std::cout, 6, true);

	std::cout << std::endl << "Maximum children subtree: " << std::endl;
	std::cout << maxChildren << " children; Tree: " << std::endl;
	maxChildrenSubtree->Serialize(std::cout, 6, true);

	return 0;
}
