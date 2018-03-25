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

#include <Windows.h>
#include "shlobj.h"

#include <cassert>
#include <cstdlib>

#include <array>
#include <fstream>

namespace Log
{

static std::ofstream sOutFile;
static const char* PATH_LOG_FILE = "LogFiles/debugLog.txt";	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb762204%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396


void Initialize(Mode mode)
{
	switch (mode)
	{
	case NONE:

		break;
	case CONSOLE:
		break;

	case Log::FILE:
	{
#if 0	// msvc unsafe
		const char* appdata = getenv("APPDATA");
#else
		PWSTR retPath = {};
		HRESULT hr = SHGetKnownFolderPath(
			FOLDERID_RoamingAppData,
			0,
			NULL,
			&retPath);
		StrUtil::UnicodeString unicodePath(retPath);
#endif
		Info(unicodePath);

		sOutFile.open(PATH_LOG_FILE);
		if (sOutFile)
		{
			sOutFile << "Log::Initialize() Done.\n";
		}
		else
		{
			Error(PATH_LOG_FILE);
		}
		break;
	}
	case CONSOLE_AND_FILE:
		break;

	default:
		break;
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
	if (sOutFile.is_open()) sOutFile << err;
}

void Warning(const std::string & s)
{
	std::string warn = "[WARNING]: ";
	warn += s; warn += "\n";
	OutputDebugString(warn.c_str());
	if (sOutFile.is_open()) sOutFile << warn;
}

void Info(const std::string & s)
{
	std::string info = s; info += "\n";
	OutputDebugString(info.c_str());
	if (sOutFile.is_open()) sOutFile << info;
}


}	// namespace Log