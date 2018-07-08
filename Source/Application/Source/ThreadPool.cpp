//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com

#include "ThreadPool.h"

#if OLD_IMPL
// TaskQueue  Worker::sTaskQueue;

void Worker::Execute()
{
	while (!mbTerminate)
	{
		Task task;
		{
			// std::unique_lock<std::mutex> lock(sTaskQueue.mutex);
			// mSignal.wait(lock, []{ return !sTaskQueue.queue.empty(); });
			// task = sTaskQueue.queue.front();
			// sTaskQueue.queue.pop();
		}
		task.func();
	}
}

const size_t ThreadPool::sHardwareThreadCount = std::thread::hardware_concurrency();


void ThreadPool::Initialize(size_t workerCount)
{	
	//mWorkers.resize(workerCount, Worker());
	for (size_t i = 0; i < workerCount; ++i)
	{
		mWorkers.emplace_back(Worker("Worker %d", static_cast<int>(i)));
	}

	//for (size_t i = 0; i < workerCount; ++i)
	//{
	//	m_pool[i]._name
	//}
}

void ThreadPool::Terminate()
{
	for (Worker& worker : mWorkers)
	{
		worker.mbTerminate = true;
	}
	
	for (Worker& worker : mWorkers)
	{
		worker.mThread.join();
	}
}

#else

using namespace VQEngine;

const size_t ThreadPool::sHardwareThreadCount = std::thread::hardware_concurrency();

ThreadPool::ThreadPool(size_t numThreads)
{
	for (auto i = 0u; i < numThreads; ++i)
	{
		mThreads.emplace_back(std::thread(&ThreadPool::Execute, this));
	}
}
ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex > lock(mMutex);
		mStopThreads = true;
	}

	mSignal.notify_all();

	for (auto& thread : mThreads)
	{
		thread.join();
	}
}

void ThreadPool::Execute()
{
	while (true)
	{
		Task task;
		{
			std::unique_lock<std::mutex > lock(mMutex);
			mSignal.wait(lock, [=] { return mStopThreads || !mTaskQueue.queue.empty(); });

			if (mStopThreads)
				break;

			task = std::move(mTaskQueue.queue.front());
			mTaskQueue.queue.pop();
		}
		task();
	}
}



#endif