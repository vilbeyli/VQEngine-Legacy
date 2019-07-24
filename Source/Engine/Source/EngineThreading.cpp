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

#include "Engine.h"

#include "Renderer/Renderer.h"

#include "Utilities/Log.h"

using namespace VQEngine;



std::mutex Engine::mLoadRenderingMutex;


#if USE_DX12


void Engine::StartThreads()
{
	Log::Info("StartingThreads...");
#if 0
	mRenderThread     = VQEngine::Thread(&Engine::RenderThread, this);
	mLoadingThread    = VQEngine::Thread(&Engine::LoadingThread, this);
	mSimulationThread = VQEngine::Thread(&Engine::SimulationThread, this);
#else
	constexpr unsigned NUM_LOADING_THREAD_WORKERS = 2;
	constexpr unsigned NUM_RENDER_THREAD_WORKERS = 4;
	constexpr unsigned NUM_SIMULATION_THREAD_WORKERS = 2;


	mRenderThread     = std::thread(&Engine::RenderThread    , this, NUM_RENDER_THREAD_WORKERS);
	mLoadingThread    = std::thread(&Engine::LoadingThread   , this, NUM_LOADING_THREAD_WORKERS);
	mSimulationThread = std::thread(&Engine::SimulationThread, this, NUM_SIMULATION_THREAD_WORKERS);
#endif
}

void Engine::StopThreads()
{
	// signal stop events
	this->mbStopThreads = true;
	mEvent_StopThreads.notify_all();

	// wait for threads to exit
	this->mRenderThread.join();
	this->mLoadingThread.join();
	this->mSimulationThread.join();
}



//
// RENDER THREAD
//
void Engine::RenderThread(unsigned numWorkers)	
{
	// Init --------------------------------------------------------------------
	ThreadPool mThreadPool(numWorkers);
	mThreadPool.StartThreads(); 

	// Loop --------------------------------------------------------------------
	while (!this->mbStopThreads)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds::duration(300));
		Log::Info("[RenderThread] Tick.");
	}


	// Exit --------------------------------------------------------------------
	Log::Info("[RenderThread] Exiting.");
}


//
// SIMULATION THREAD
//
void Engine::SimulationThread(unsigned numWorkers)
{
	// Init --------------------------------------------------------------------
	ThreadPool mThreadPool(numWorkers);
	mThreadPool.StartThreads();

	// Loop --------------------------------------------------------------------
	while (!this->mbStopThreads)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds::duration(98));
		Log::Info("[SimulationThread] Tick.");

	}

	// Exit --------------------------------------------------------------------
	Log::Info("[SimulationThread] Exiting.");
}


//
// LOADING THREAD
//
void Engine::LoadingThread(unsigned numWorkers)
{
	// Init --------------------------------------------------------------------
	// TODO: shall we be using the thread context for this? or is it better to 
	//       move the ThreadPool to engine and keep a pointer for a lighter context?
	ThreadPool mThreadPool(numWorkers); 
	mThreadPool.StartThreads();


	// Loop --------------------------------------------------------------------
	// TODO: this looks pretty much like a copy of threadpool code...
	Task task;
	while (!this->mbStopThreads)
	{
		///std::this_thread::sleep_for(std::chrono::milliseconds::duration(2500));
		///Log::Info("[LoadingThread] Tick.");
		{
			std::unique_lock<std::mutex> lock(mEngineLoadingTaskQueue.mutex);
			mEvent_StopThreads.wait(lock, [=] { return this->mbStopThreads || !mEngineLoadingTaskQueue.queue.empty(); });

			// if mbStopThreads were issued, abort the current task and exit right away
			if (this->mbStopThreads)
			{
				break;
			}

			mEngineLoadingTaskQueue.GetTaskFromTopOfQueue(task);
		}

		// TODO: distribute to the workers.
		// execute the task 
		task();
	}

	// Exit --------------------------------------------------------------------
	Log::Info("[LoadingThread] Exiting.");
}

#else


void Engine::StartRenderThread()
{
	mbStopRenderThread = false;
	mRenderThread = std::thread(&Engine::RenderThread, this, 4);
}


void Engine::StopRenderThreadAndWait()
{
	mbStopRenderThread = true;
	mSignalRender.notify_all();
	if(mRenderThread.joinable())
		mRenderThread.join();
}


void Engine::RenderThread(unsigned numWorkers)	// This thread is currently only used during loading.
{
	constexpr bool bOneTimeLoadingScreenRender = false; // We're looping;
	while (!mbStopRenderThread)
	{
		std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
		mSignalRender.wait(lck); /// , [=]() { return !mbStopRenderThread; });


#if USE_DX12
		// TODO-DX12: GET SWAPCHAIN IMAGE
		if (mbLoading)
		{
			continue;
		}
		mpCPUProfiler->BeginEntry("CPU");

		mpGPUProfiler->BeginProfile(mFrameCount);
		mpGPUProfiler->BeginEntry("GPU");

		mpRenderer->BeginFrame();
#else
		mpCPUProfiler->EndProfile();
		mpCPUProfiler->BeginProfile();
		mpCPUProfiler->BeginEntry("CPU");

		mpGPUProfiler->BeginProfile(mFrameCount);
		mpGPUProfiler->BeginEntry("GPU");

		mpRenderer->BeginFrame();
#endif

		RenderLoadingScreen(bOneTimeLoadingScreenRender);
		RenderUI();

		mpGPUProfiler->EndEntry();	// GPU
		mpGPUProfiler->EndProfile(mFrameCount);


		mpCPUProfiler->BeginEntry("Present");
#if USE_DX12
		// TODO-DX12: PRESENT QUEUE
#else
		mpRenderer->EndFrame();
#endif
		mpCPUProfiler->EndEntry(); // Present


		mpCPUProfiler->EndEntry();	// CPU
		///fmpCPUProfiler->StateCheck();
		mAccumulator = 0.0f;
		++mFrameCount;
	}

	// when loading is finished, we use the same signal variable to signal
	// the update thread (which woke this thread up in the first place)
	mpCPUProfiler->Clear();
	mpGPUProfiler->Clear();
	mSignalRender.notify_all();
}


#endif
