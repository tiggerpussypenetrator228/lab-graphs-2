#pragma once

#include <cstdlib>

#include <chrono>

// Перегружаем оператор new. Таким образом мы сможем отследить использование памяти в нашем коде.
void* operator new(size_t bytes);

// Так же перенаправляем вызовы malloc на нашу имплементацию.
void* __malloc(size_t bytes);
#define malloc __malloc

namespace profile
{
	// Функции профилирования памяти.

	void StartMemoryProfiling();
	void EndMemoryProfiling();

	size_t GetProfiledMemory();

	// Функции профилирования времени.

	void StartTimeProfiling();
	void EndTimeProfiling();

	std::chrono::microseconds GetProfiledTime();
}
