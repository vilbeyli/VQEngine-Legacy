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


#include "VQUI.h"

#include "Utilities/Log.h"

#include "Application/ThreadPool.h"

#include <thread>
#include <assert.h>

using namespace VQEngine;



const char* VQEngine::VQUI::pDLLName = "VQUI.dll";

void VQEngine::VQUI::ShowWindow0() const { auto fnLaunchWnd = [&]() { pFnShowWindow(mHControlPanel0); }; mpThreadPool->AddTask(fnLaunchWnd); }
void VQEngine::VQUI::ShowWindow1() const { auto fnLaunchWnd = [&]() { pFnShowWindow(mHControlPanel1); }; mpThreadPool->AddTask(fnLaunchWnd); }
void VQEngine::VQUI::ShowWindow2() const { auto fnLaunchWnd = [&]() { pFnShowWindow(mHControlPanel2); }; mpThreadPool->AddTask(fnLaunchWnd); }
void VQEngine::VQUI::ShowWindow3() const { auto fnLaunchWnd = [&]() { pFnShowWindow(mHControlPanel3); }; mpThreadPool->AddTask(fnLaunchWnd); }

bool VQUI::Initialize(std::string& errMsg)
{
	mpThreadPool = new ThreadPool(4);

	// Load the DLL
	mHModule = LoadLibrary(TEXT(VQUI::pDLLName));
	if (mHModule == NULL)
	{
		errMsg = std::string("LoadLibrary() failed for " + std::string(VQUI::pDLLName));
		return false;
	}

	// get the Function Address for TestFn
#if 0
	const char* pFnName = "TestFn";
#else
	const char* pFnName = "LaunchWindow";
#endif
	using pfnTestFn = void(*)();
	pfnTestFn TestFn = (pfnTestFn)GetProcAddress(mHModule, pFnName);
	if (TestFn == NULL)
	{
		errMsg = std::string(std::string(pFnName) + " doesn't exist in ProcAddress");
		//FreeLibrary(mHModule);
		//return false;
	}

#if 0
	// test launching window
	std::thread th(TestFn);
	Log::Info("Sleeping...");
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	std::thread th2(TestFn);
	Log::Info("Woke Up");
	th.join();
	th2.join();
#else
#if 1
	// test launching windows with separate data using handles
	pFnCreateWindow = (int(*)(int))  GetProcAddress(mHModule, "CreateControlPanel");
	pFnShowWindow = (void(*)(int)) GetProcAddress(mHModule, "ShowControlPanel");
	pFnShutdownWindows = (void(*)(void)) GetProcAddress(mHModule, "ShutdownWindows");
	pFnAddSliderFToControlPanel = (void(*)(int, SliderDescData)) GetProcAddress(mHModule, "AddSliderFToControlPanel");

	if (pFnCreateWindow == NULL || pFnShowWindow == NULL || pFnShutdownWindows == NULL || pFnAddSliderFToControlPanel == NULL)
	{
		assert(false);
	}

	// test w/ random numbers
	mHControlPanel0 = pFnCreateWindow(42);
	mHControlPanel1 = pFnCreateWindow(154);
	mHControlPanel2 = pFnCreateWindow(3);
	mHControlPanel3 = pFnCreateWindow(14);

	//desc.label = "Test Float";
	//strcpy_s(mDesc.label, "vfTest Float");
	wcscpy_s(mSliderDescTest.label, L"vfTest Float");
	testFloat = 0.1f;
	mSliderDescTest.pData = &testFloat;
	pFnAddSliderFToControlPanel(mHControlPanel0, mSliderDescTest);

	// TODO: fix mouse capture / focus steal with this window launch.
	this->ShowWindow0();
	//	this->ShowWindow1();
	//	this->ShowWindow3();
	//	this->ShowWindow2();
#else

#endif
#endif


	return true;
}

void VQUI::Exit()
{
	pFnShutdownWindows();
	delete mpThreadPool;
	FreeLibrary(mHModule);
}


