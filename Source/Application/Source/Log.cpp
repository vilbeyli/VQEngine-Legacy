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
