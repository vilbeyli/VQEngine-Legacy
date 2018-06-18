//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2016  - Volkan Ilbeyli
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

#pragma once

#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <queue>
#include <future>
#include <condition_variable>

// http://www.cplusplus.com/reference/thread/thread/
// https://stackoverflow.com/a/32593825/2034041
// todo: finish implementation for shader hotswapping

using Task = std::function<void()>;

struct TaskQueue
{
	std::mutex		 mutex;
	std::queue<Task> queue;
};


#define OLD_IMPL 0
#if OLD_IMPL

class Worker
{
	friend class ThreadPool;
private:
	std::string		mName;
	std::thread		mThread;
	bool			mbTerminate;

	std::mutex				mMutex;
	std::condition_variable mSignal;

	void Execute();

public:
	
	template<class... Args>
	Worker(const char* format, Args&&... args)
		:
		mbTerminate(false),
		mThread(&Worker::Execute, this)
	{
		char name[256];
		sprintf_s(name, format, args...);
		mName = name;
	}

	inline const std::string& Name() const { return mName; }
};

class ThreadPool
{
public:
	static const size_t sHardwareThreadCount;

	ThreadPool() = default;
	~ThreadPool() = default;

	void Initialize(size_t workerCount);
	void Terminate();

	void AddTask();
private:
	std::vector<Worker> mWorkers;
	TaskQueue			mTaskQueue;
};

#else

namespace VQEngine
{
	// src: https://www.youtube.com/watch?v=eWTGtp3HXiw

	class ThreadPool
	{
	public:
		const static size_t ThreadPool::sHardwareThreadCount;

		ThreadPool(size_t numThreads);
		~ThreadPool();


		// Notes on C++11 Threading:
		// ------------------------------------------------------------------------------------
		// 
		// To get a return value from an executing thread, we use std::packaged_task<> together
		// with std::future to access the results later.
		//
		// e.g. http://en.cppreference.com/w/cpp/thread/packaged_task
		//
		// int f(int x, int y) { return std::pow(x,y); }
		//
		// 	std::packaged_task<int(int, int)> task([](int a, int b) {
		// 		return std::pow(a, b);
		// 	});
		// 	std::future<int> result = task.get_future();
		// 	task(2, 9);
		//
		// *****
		// 
		// 	std::packaged_task<int()> task(std::bind(f, 2, 11));
		// 	std::future<int> result = task.get_future();
		// 	task();
		// 
		// *****
		//
		// 	std::packaged_task<int(int, int)> task(f);
		// 	std::future<int> result = task.get_future(); 
		// 	std::thread task_td(std::move(task), 2, 10);
		// 
		// ------------------------------------------------------------------------------------


		// Adds a task to the thread pool and returns the std::future<> 
		// containing the return type of the added task.
		//
		template<class T>
		//std::future<decltype(task())> AddTask(T task)	// (why doesn't this compile)
		auto AddTask(T task) -> std::future<decltype(task())>
		{
			// use a shared_ptr<> of packaged tasks here as we execute them in the thread pool workers as well
			// as accesing its get_future() on the thread that calls this AddTask() function.
			using typename task_return_t = decltype(task());
			auto pTask = std::make_shared< std::packaged_task<task_return_t()>>(std::move(task));
			{
				std::unique_lock<std::mutex> lock(mMutex);
				mTaskQueue.queue.emplace([=]
				{					// Add a lambda function to the task queue which 
					(*pTask)();		// calls the packaged_task<>'s callable object -> T task 
				});
			}

			mSignal.notify_one();
			return pTask->get_future();
		}

	private:
		void Execute();

		std::vector<std::thread>	mThreads;
		std::condition_variable		mSignal;
		std::mutex					mMutex;
		bool						mStopThreads = false;

		TaskQueue					mTaskQueue;
	};


}
#endif