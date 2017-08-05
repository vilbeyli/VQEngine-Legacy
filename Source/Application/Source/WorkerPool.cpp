#include "WorkerPool.h"


TaskQueue  Worker::sTaskQueue;

void Worker::Execute()
{
	while (!_bTerminate)
	{
		Task task;
		{
			std::unique_lock<std::mutex> lock(sTaskQueue._mutex);
			_signal.wait(lock, []{ return !sTaskQueue._queue.empty(); });
			task = sTaskQueue._queue.front();
			sTaskQueue._queue.pop();
		}
		task._function();
	}
}

//WorkerPool::WorkerPool()
//	:
//	m_pool(std::thread::hardware_concurrency())
//{
//}

void WorkerPool::Initialize(size_t workerCount)
{	
	//for (size_t i = 0; i < workerCount; ++i)
	//{
	//	m_pool[i]._name
	//}
}

void WorkerPool::Terminate()
{
	for (Worker& worker : m_pool)
	{
		worker._bTerminate = true;
	}
	
	for (Worker& worker : m_pool)
	{
		worker._thread.join();
	}
}

