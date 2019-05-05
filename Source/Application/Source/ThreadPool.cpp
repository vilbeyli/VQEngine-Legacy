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
#include "Utilities/Utils.h"
#include "Utilities/Log.h"

using namespace VQEngine;

const size_t ThreadPool::sHardwareThreadCount = std::thread::hardware_concurrency();


ThreadPool::ThreadPool(size_t numThreads)
{
	for (auto i = 0u; i < numThreads; ++i)
	{
		mThreads.emplace_back(std::thread(&ThreadPool::Execute, this));
	}

	// Thread Pool Unit Test ------------------------------------------------
	if (false)
	{
		constexpr long long sz = 40000000;
		auto sumRnd = [&]()
		{
			std::vector<long long> nums(sz, 0);
			for (int i = 0; i < sz; ++i)
			{
				nums[i] = MathUtil::RandI(0, 5000);
			}
			unsigned long long result = 0;
			for (int i = 0; i < sz; ++i)
			{
				if (nums[i] > 3000)
					result += nums[i];
			}
			return result;
		};
		auto sum = [&]()
		{
			std::vector<long long> nums(sz, 0);
			for (int i = 0; i < sz; ++i)
			{
				nums[i] = MathUtil::RandI(0, 5000);
			}
			unsigned long long result = 0;
			for (int i = 0; i < sz; ++i)
			{
				result += nums[i];
			}
			return result;
		};

		constexpr int threadCount = 16;
		std::future<unsigned long long> futures[threadCount] =
		{
			this->AddTask(sumRnd),
			this->AddTask(sumRnd),
			this->AddTask(sumRnd),
			this->AddTask(sumRnd),
			this->AddTask(sumRnd),
			this->AddTask(sumRnd),
			this->AddTask(sumRnd),
			this->AddTask(sumRnd),

			this->AddTask(sum),
			this->AddTask(sum),
			this->AddTask(sum),
			this->AddTask(sum),
			this->AddTask(sum),
			this->AddTask(sum),
			this->AddTask(sum),
			this->AddTask(sum),
		};

		std::vector<unsigned long long> results;
		unsigned long long total = 0;
		std::for_each(std::begin(futures), std::end(futures), [&](decltype(futures[0]) f)
		{
			results.push_back(f.get());
			total += results.back();
		});

		std::string strResult = "total (" + std::to_string(total) + ") = ";
		for (int i = 0; i < threadCount; ++i)
		{
			strResult += "(" + std::to_string(results[i]) + ") " + (i == threadCount - 1 ? "" : " + ");
		}
		Log::Info(strResult);
	}
	// Thread Pool Unit Test ------------------------------------------------
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
