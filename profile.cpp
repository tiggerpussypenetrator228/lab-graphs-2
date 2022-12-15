#include "profile.hpp"

// Глобальные переменные для профилирования памяти.
size_t CapturedMemory = 0;
bool ShouldCaptureMemory = false;

// Глобальные переменные для профилирования времени.
std::chrono::high_resolution_clock::time_point StartTime;
std::chrono::high_resolution_clock::duration CapturedTime;

// В этом файле нам не нужно перенаправлять вызовы malloc на нашу имплементацию.
#undef malloc

// Если мы захватываем (профилируем) память, то добавить количество выделяемых байт к счётчику.
void* operator new(size_t bytes)
{
	if (ShouldCaptureMemory)
	{
		CapturedMemory += bytes;
	}

	return malloc(bytes);
}

// Тут то же самое, что и выше.
void* __malloc(size_t bytes)
{
	if (ShouldCaptureMemory)
	{
		CapturedMemory += bytes;
	}

	return malloc(bytes);
}

namespace profile
{
	// Обнуляем счётчик байт и устанавливаем флаг для начала счёта.
	void StartMemoryProfiling()
	{
		CapturedMemory = 0;
		ShouldCaptureMemory = true;
	}

	// Устанавливаем флаг для конца счёта.
	void EndMemoryProfiling()
	{
		ShouldCaptureMemory = false;
	}

	// Получение запрофилированной памяти.
	size_t GetProfiledMemory()
	{
		return CapturedMemory;
	}

	// Устанавливаем начальную точку времени, получая текущее время через high resolution clock.
	void StartTimeProfiling()
	{
		StartTime = std::chrono::high_resolution_clock::now();
	}

	/* 
		Отнимаем от текущего времени начальную точку, таким образом получая разницу 
		во времени между вызовами StartTimeProfiling и EndTimeProfiling
	*/
	void EndTimeProfiling()
	{
		CapturedTime = std::chrono::high_resolution_clock::now() - StartTime;
	}

	// Получение запрофилированного времени. Здесь происходит каст к микросекундам.
	std::chrono::microseconds GetProfiledTime()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(CapturedTime);
	}
}
