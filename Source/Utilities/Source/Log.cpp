//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018 - Volkan Ilbeyli
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

#include "Log.h"
#include "utils.h"

#include <fstream>
#include <iostream>

#include <fcntl.h>
#include <io.h>

#include <Windows.h>


namespace Log
{

	static std::ofstream sOutFile;
	using namespace std;


static const WORD MAX_CONSOLE_LINES = 500;


std::string InitLogFile()
{
	const std::string EngineWorkspaceDir = DirectoryUtil::GetSpecialFolderPath(DirectoryUtil::ESpecialFolder::APPDATA) + "\\VQEngine";
	const std::string LogFileDir = EngineWorkspaceDir + "\\Logs";

	std::string errMsg = "";
	if (CreateDirectory(EngineWorkspaceDir.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError())
	{
		if (CreateDirectory(LogFileDir.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError())
		{
			std::string fileName = GetCurrentTimeAsString() + "_VQEngineLog.txt";
			sOutFile.open(LogFileDir + "\\" + fileName);
			if (sOutFile)
			{
				sOutFile << GetCurrentTimeAsStringWithBrackets() + "[Log] " << "Initialize() Done.";
			}
			else
			{
				errMsg = "Cannot open log file " + fileName;
			}
		}
		else
		{
			errMsg = "Failed to create directory " + LogFileDir;
		}
	}
	else
	{
		errMsg = "Failed to create directory " + EngineWorkspaceDir;
	}
	return errMsg;
}
void InitConsole()
{
	// src: https://stackoverflow.com/a/46050762/2034041
	//void RedirectIOToConsole() 
	{
		//Create a console for this application
		AllocConsole();

		// Get STDOUT handle
		HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
		std::FILE *COutputHandle = _fdopen(SystemOutput, "w");

		// Get STDERR handle
		HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
		int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
		std::FILE *CErrorHandle = _fdopen(SystemError, "w");

		// Get STDIN handle
		HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
		int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
		std::FILE *CInputHandle = _fdopen(SystemInput, "r");

		//make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
		ios::sync_with_stdio(true);

		// Redirect the CRT standard input, output, and error handles to the console
		freopen_s(&CInputHandle, "CONIN$", "r", stdin);
		freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
		freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);

		//Clear the error state for each of the C++ standard stream objects. We need to do this, as
		//attempts to access the standard streams before they refer to a valid target will cause the
		//iostream objects to enter an error state. In versions of Visual Studio after 2005, this seems
		//to always occur during startup regardless of whether anything has been read from or written to
		//the console or not.
		std::wcout.clear();
		std::cout.clear();
		std::wcerr.clear();
		std::cerr.clear();
		std::wcin.clear();
		std::cin.clear();
	}
}

void Initialize(Mode mode)
{
	std::string errMsg = "";
	switch (mode)
	{
	case NONE:

		break;
	case CONSOLE:
		InitConsole();
		break;

	case Log::FILE:
		errMsg = InitLogFile();
		break;
	
	case CONSOLE_AND_FILE:
		InitConsole();
		errMsg = InitLogFile();
		break;

	default:
		break;
	}

	if (!errMsg.empty())
	{
		MessageBox(NULL, errMsg.c_str(), "VQEngine: Error Initializing Logging", MB_OK);
	}
}

void Exit()
{
	std::string msg = GetCurrentTimeAsStringWithBrackets() + "[Log] Exit()";
	if (sOutFile.is_open())
	{
		sOutFile << msg;
		sOutFile.close();
	}
	cout << msg;
	OutputDebugString(msg.c_str());
}

void Error(const std::string & s)
{
	std::string err = GetCurrentTimeAsStringWithBrackets() + "[ERROR]: ";
	err += s + "\n";
	
	OutputDebugString(err.c_str());		// vs
	if (sOutFile.is_open()) 
		sOutFile << err;				// file
	cout << err;						// console
}

void Warning(const std::string & s)
{
	std::string warn = GetCurrentTimeAsStringWithBrackets() + "[WARNING]: ";
	warn += s + "\n";
	
	OutputDebugString(warn.c_str());
	if (sOutFile.is_open()) 
		sOutFile << warn;
	cout << warn;
}

void Info(const std::string & s)
{
	std::string info = GetCurrentTimeAsStringWithBrackets() + "[INFO]:" + s + "\n";
	OutputDebugString(info.c_str());
	
	if (sOutFile.is_open()) 
		sOutFile << info;
	cout << info;
}

}	// namespace Log