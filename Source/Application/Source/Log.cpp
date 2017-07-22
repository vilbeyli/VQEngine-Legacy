#include "Log.h"

#include <array>
#include <Windows.h>
#include <cassert>

const std::array<const char*, ERROR_LOG::ERR_LOG_COUNT> s_errorStrings =
{
	"Can't open file "
};

const char* debugOutputPath = "";
Log::Log()
{
	m_outFile.open(debugOutputPath);
	if (m_outFile)
	{

	}
	else
	{
		this->Error(ERROR_LOG::CANT_OPEN_FILE, debugOutputPath);
	}
}


Log::~Log()
{
	m_outFile.close();
}

void Log::Error(ERROR_LOG errMode, const std::string& s)
{
	std::string err("\n***** ERROR: ");
	err += s_errorStrings[errMode] + s;
	OutputDebugString(err.c_str());
}

void Log::Error(const std::string & s)
{
	std::string err("\n***** ERROR: ");
	err += s;
	OutputDebugString(err.c_str());
}

void Log::Info(const std::string & s)
{
	std::string info("\n----- INFO : ");
	info += s;
	OutputDebugString(info.c_str());
}
