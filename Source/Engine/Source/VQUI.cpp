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

#include <thread>
#include <assert.h>

using namespace VQEngine;



const char* VQEngine::VQUI::pDLLName = "VQUI.dll";

bool VQUI::Initialize(std::string& errMsg)
{
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
		FreeLibrary(mHModule);
		return false;
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
	pFnShowWindow   = (void(*)(int)) GetProcAddress(mHModule, "ShowControlPanel");

	if (pFnCreateWindow == NULL || pFnShowWindow == NULL)
	{
		assert(false);
	}


	mHControlPanel0 = pFnCreateWindow(42);
	mHControlPanel1 = pFnCreateWindow(154);

	std::thread th(pFnShowWindow, mHControlPanel0);
	Log::Info("Sleeping...");
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	std::thread th2(pFnShowWindow, mHControlPanel1);
	Log::Info("Woke Up");
	th.join();
	th2.join();
#else

#endif
#endif


	return true;
}

void VQUI::Exit()
{
	FreeLibrary(mHModule);
}


