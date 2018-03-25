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

#include <cassert>
#include <cstdlib>

#include <array>
#include <fstream>

#include <Windows.h>


namespace Log
{

static std::ofstream sOutFile;




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
				sOutFile << "[" << GetCurrentTimeAsString() << "] [Log]:" << "Initialize() Done.";
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
std::string InitConsole()
{
	std::string errMsg = "";

	return errMsg;
}

void Initialize(Mode mode)
{
	std::string errMsg = "";
	switch (mode)
	{
	case NONE:

		break;
	case CONSOLE:
		errMsg = InitConsole();
		break;

	case Log::FILE:
		errMsg = InitLogFile();
		break;
	
	case CONSOLE_AND_FILE:
		errMsg = InitLogFile() + InitConsole();
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
	if (sOutFile.is_open())
	{
		sOutFile << "Log::Exit().\n";
		sOutFile.close();
	}
}

void Error(const std::string & s)
{
	std::string err("\n***** ERROR: ");
	err += s;
	
	OutputDebugString(err.c_str());
	if (sOutFile.is_open()) 
		sOutFile << err;
}

void Warning(const std::string & s)
{
	std::string warn = "[WARNING]: ";
	warn += s; 
	warn += "\n";
	
	OutputDebugString(warn.c_str());
	if (sOutFile.is_open()) sOutFile << warn;
}

void Info(const std::string & s)
{
	std::string info = s; 
	info += "\n";
	OutputDebugString(info.c_str());
	
	if (sOutFile.is_open()) sOutFile << info;
}


}	// namespace Log