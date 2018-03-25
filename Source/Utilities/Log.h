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

#include <string>

#define VARIADIC_LOG_FN(FN_NAME)\
template<class... Args>\
void FN_NAME(const char* format, Args&&... args)\
{\
	char msg[LEN_MSG_BUFFER];\
	sprintf_s(msg, format, args...);\
	##FN_NAME(std::string(msg));\
}

namespace Log
{
	enum Mode : unsigned
	{
		NONE				= 0,	// Visual Studio Output Window
		CONSOLE				= 1,	// Separate Console Window
		FILE				= 2,	// Log File in %APPDATA%\VQEngine\DATE-VQEngineLog.txt
		CONSOLE_AND_FILE	= 3,	// Both Console Window & Log File
	};
	//-------------------------------------

	constexpr size_t LEN_MSG_BUFFER = 2048;
	

	void Initialize(Mode mode);
	void Exit();

	void Info(const std::string& s);
	void Error(const std::string& s);
	void Warning(const std::string& s);
	
	VARIADIC_LOG_FN(Error)
	VARIADIC_LOG_FN(Warning)
	VARIADIC_LOG_FN(Info)
}