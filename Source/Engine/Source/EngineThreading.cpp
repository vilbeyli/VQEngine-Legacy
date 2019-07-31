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

#define DEBUG_LOG_THREAD_TICKS 1

void Engine::StartThreads()
{
	Log::Info("StartingThreads...");
#if 0
	mRenderThread     = VQEngine::Thread(&Engine::RenderThread, this);
	mLoadingThread    = VQEngine::Thread(&Engine::LoadingThread, this);
	mSimulationThread = VQEngine::Thread(&Engine::SimulationThread, this);
#else
	constexpr unsigned NUM_RENDER_THREAD_WORKERS = 4;
	constexpr unsigned NUM_SIMULATION_THREAD_WORKERS = 2;

	// TODO: pass threadpool pointers instead of allocating them on stack
	mRenderThread     = std::thread(&Engine::RenderThread    , this, NUM_RENDER_THREAD_WORKERS);
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
	this->mSimulationThread.join();
}



//
// RENDER THREAD
//
void Engine::RenderThread(unsigned numWorkers)	
{
	// wait for the initial load before rendering a loading screen.
	// this is expected to happen after renderer is initialized and
	// first rendering screen texture is loaded.
	{
		std::mutex _mtxLoadingScreen;
		std::unique_lock<std::mutex> lock(_mtxLoadingScreen);
		mEvent_LoadingScreenReady.wait(lock);
	}
	this->RenderLoadingScreen();


	// Loop --------------------------------------------------------------------
	while (!this->mbStopThreads)
	{
		{
			std::unique_lock<std::mutex> lock(mEventMutex_FrameUpdateFinished);
			mEvent_FrameUpdateFinished.wait(lock);
		}

#if DEBUG_LOG_THREAD_TICKS
		Log::Info("[RenderThread] Tick.");
#endif

		// render loading screen if we're currently loading
		if (this->mbLoading)
		{
			this->RenderLoadingScreen();

			// signal update thread that frame is done rendering
			mEvent_FrameRenderFinished.notify_one();
			continue;
		}

		// render frame TODO:
		Log::Info("----- RenderFrame.");


		// signal update thread that frame is done rendering
		mEvent_FrameRenderFinished.notify_one();
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
#if DEBUG_LOG_THREAD_TICKS
		Log::Info("[SimulationThread] Tick.");
#endif

		// TODO: wait for render to finish / acquire image or something

		// TODO: update
		std::this_thread::sleep_for(std::chrono::milliseconds::duration(16 * 100));

		// signal render thread to kick off rendering
		mEvent_FrameUpdateFinished.notify_one();
	}

	// Exit --------------------------------------------------------------------
	Log::Info("[SimulationThread] Exiting.");
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
