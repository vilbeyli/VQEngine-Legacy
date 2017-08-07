#pragma once

#include <string>
#include <fstream>

enum ERROR_LOG
{
	CANT_OPEN_FILE,
	CANT_CREATE_RESOURCE,
	CANT_CRERATE_RENDER_STATE,

	ERR_LOG_COUNT
};

class Log
{
public:
	Log()  = delete;
	~Log() = delete;

	static void Info(const std::string& s);
	static void Error(ERROR_LOG errMode, const std::string& s);
	static void Error(const std::string& s);

	// functions for printf() style logging using variadic templates
	template<class... Args> 
	static void Error(const char* format, Args&&... args)
	{
		char msg[256];	sprintf_s(msg, format, args...);
		Error(std::string(msg));
	}

	template<class... Args>
	static void Info(const char* format, Args&&... args)
	{
		char msg[256];	sprintf_s(msg, format, args...);
		Info(std::string(msg));
	}
private:
	std::ofstream m_outFile;	
};

