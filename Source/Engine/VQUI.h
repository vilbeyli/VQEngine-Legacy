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

#pragma once

#include <windows.h>
#include <string>

namespace VQEngine { class ThreadPool; }

namespace VQEngine
{
	class VQUI
	{
	public:
		static const char* pDLLName;

		bool Initialize(std::string& errMsgIn);
		void Exit();

		
		// currently running windows on class's own threadpool.
		//
		// due to the limitation of the threading system ( no
		// function parameters supported when using AddTask() )
		// using dedicated functions for now.
		void ShowWindow0() const;
		void ShowWindow1() const; // unused
		void ShowWindow2() const; // unused
		void ShowWindow3() const; // unused

		// UI STRUCTS
		//
		
		// needs to match the C# struct
		#define UI_LABEL_SIZE 256 
		struct SliderDescData
		{
			float* pData;
			wchar_t label[UI_LABEL_SIZE];
		};


		float testFloat = 0.0f;
	private:

		HMODULE mHModule; // DLL module handle

		// need our own thread pool, Application's won't work.
		ThreadPool* mpThreadPool;

		// test desc
		SliderDescData mSliderDescTest;

		// test windows
		int mHControlPanel0;
		int mHControlPanel1;
		int mHControlPanel2;
		int mHControlPanel3;

		// imported functions from C#
		void (*pFnShowWindow)(int);
		int  (*pFnCreateWindow)(int data);
		void (*pFnShutdownWindows)();
		void (*pFnAddSliderFToControlPanel)(int hPanel, SliderDescData pDesc);
	};
}

constexpr size_t t = sizeof(VQEngine::VQUI::SliderDescData);