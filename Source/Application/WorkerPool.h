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
#include <condition_variable>

// http://www.cplusplus.com/reference/thread/thread/
// https://stackoverflow.com/a/32593825/2034041
// todo: finish implementation for shader hotswapping

struct Task
{
	std::function<void()> _function;
};

struct TaskQueue
{
	std::mutex		 _mutex;
	std::queue<Task> _queue;
};

class Worker
{
	friend class WorkerPool;
private:
	std::string		_name;
	std::thread		_thread;
	bool			_bTerminate;

	std::mutex				_mutex;
	std::condition_variable _signal;

	void Execute();

public:
	static TaskQueue  sTaskQueue;
	
	
	template<class... Args>
	Worker(const char* format, Args&&... args)
		:
		_bTerminate(false),
		_thread(&Worker::Execute, this)
	{
		char name[256];
		sprintf_s(name, format, args...);
		_name = name;
	}

	inline const std::string& Name() const { return _name; }
};

class WorkerPool
{
public:
	WorkerPool() = default;
	void Initialize(size_t workerCount);
	void Terminate();

	void AddTask();

private:
	std::vector<Worker> m_pool;
};

