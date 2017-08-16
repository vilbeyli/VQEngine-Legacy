//	DX11Renderer - VDemo | DirectX11 Renderer
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

#include "Log.h"

#include <array>
#include <Windows.h>
#include <cassert>

const std::array<const char*, ERROR_LOG::ERR_LOG_COUNT> s_errorStrings =
{
	"Can't open file ",
	"Creating rendering resource: ",
	"Creating render state: "
};

std::ofstream Log::sOutFile;

const char* debugOutputPath = "Debug/debugLog.txt";
void Log::Initialize(bool bEnableLogging)
{
	if (bEnableLogging)
	{
		sOutFile.open(debugOutputPath);
		if (sOutFile)
		{
			sOutFile << "Log::Initialize() Done.\n";
		}
		else
		{
			Error(ERROR_LOG::CANT_OPEN_FILE, debugOutputPath);
		}
	}
}

void Log::Exit()
{
	if (sOutFile.is_open())
	{
		sOutFile << "Log::Exit().\n";
		sOutFile.close();
	}
}


void Log::Error(ERROR_LOG errMode, const std::string& s)
{
	std::string err("\n***** ERROR: ");
	err += s_errorStrings[errMode] + s;
	OutputDebugString(err.c_str());
	if (sOutFile.is_open()) sOutFile << err;
}

void Log::Error(const std::string & s)
{
	std::string err("\n***** ERROR: ");
	err += s;
	OutputDebugString(err.c_str());
	if (sOutFile.is_open()) sOutFile << err;
}

void Log::Info(const std::string & s)
{
	std::string info("\n----- INFO : ");
	info += s; info += "\n";
	OutputDebugString(info.c_str());
	if (sOutFile.is_open()) sOutFile << info;
}
