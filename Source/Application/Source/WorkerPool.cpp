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

