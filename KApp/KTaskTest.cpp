#define MEMORY_DUMP_DEBUG
#include "KBase/Interface/IKMemory.h"
#include "KBase/Publish/KEvent.h"
#include "KBase/Interface/Task/IKAsyncTask.h"
#include "KBase/Interface/Task/IKTaskGraph.h"
#include "KBase/Internal/Task/KTaskThreadPool.h"
#include <functional>
#include <algorithm>

KEvent event;

int fibonacci(int n)
{
	if (n <= 1) return n;

	std::vector<int> fib(n + 1);
	fib[0] = 0;
	fib[1] = 1;

	for (int i = 2; i <= n; i++) {
		fib[i] = fib[i - 1] + fib[i - 2];
	}

	return fib[n];
}

void quickSort(std::vector<int>& arr, int left, int right)
{
	int i = left, j = right;
	int tmp;
	int pivot = arr[(left + right) / 2];

	while (i <= j) {
		while (arr[i] < pivot)
			i++;
		while (arr[j] > pivot)
			j--;
		if (i <= j) {
			tmp = arr[i];
			arr[i] = arr[j];
			arr[j] = tmp;
			i++;
			j--;
		}
	};

	if (left < j)
		quickSort(arr, left, j);
	if (i < right)
		quickSort(arr, i, right);
}

void sort()
{
	std::vector<int> values(1000000);
	std::generate(values.begin(), values.end(), std::rand);
	quickSort(values, 0, (int)values.size() - 1);
}

struct Task
{
	uint32_t counter;
	std::string name;
	Task(uint32_t c)
	{
		counter = c;
		name = "Task_" + std::to_string(counter);
	}
	void DoWork()
	{
		fibonacci(100);
		printf("Task_%d\n", counter);
	}
	void Abandon() {}

	const char* GetDebugInfo()
	{
		return name.c_str();
	}
};

struct Task2
{
	uint32_t counter2;
	std::string name;
	Task2(uint32_t c)
	{
		counter2 = c;
		name = "Task2_" + std::to_string(counter2);
	}
	void DoWork()
	{
		fibonacci(10);
		printf("Task2_%d\n", counter2);
	}
	void Abandon() {}

	const char* GetDebugInfo()
	{
		return name.c_str();
	}
};

struct Task3
{
	uint32_t counter3;
	std::string name;
	Task3(uint32_t c)
	{
		counter3 = c;
		name = "Task3_" + std::to_string(counter3);
	}
	void DoWork()
	{
		fibonacci(10000000);
		printf("Task3_%d\n", counter3);
	}
	void Abandon() {}

	const char* GetDebugInfo()
	{
		return name.c_str();
	}
};

int main()
{
	DUMP_MEMORY_LEAK_BEGIN();

	GetAsyncTaskManager()->Init();
	GetTaskGraphManager()->Init();

	GetTaskGraphManager()->AttachToThread(NamedThread::GAME_THREAD);

	ParallelFor([](uint32_t index)
	{
		fibonacci(1000000);
		printf("Task_%d\n", index);
	}, 30000);

	GetTaskGraphManager()->UnInit();
	GetAsyncTaskManager()->UnInit();

	return 0;
}