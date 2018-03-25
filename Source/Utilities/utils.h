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

#ifndef UTILS_CPP
#define UTILS_CPP

#include <string>
#include <vector>

// typedefs
typedef wchar_t WCHAR;    // wc,   16-bit UNICODE character
typedef wchar_t* PWSTR;

/// STRING PROCESSING
//===============================================================================================
namespace StrUtil
{
	std::vector<std::string> split(const char* s, char c = ' ');
	std::vector<std::string> split(const std::string& s, char c = ' ');
	std::vector<std::string> split(const std::string& s, const std::vector<char>& delimiters);

	template<class... Args> std::vector<std::string> split(const std::string& s, Args&&... args)
	{
		const std::vector<char> delimiters = { args... };
		return split(s, delimiters);
	}

	std::string	GetFileNameWithoutExtension(const std::string&);
	bool IsImageName(const std::string&);

	struct UnicodeString
	{
	private:
		std::string  str;
		std::wstring wstr;

	public:
		UnicodeString(const std::string& strIn);
		UnicodeString(PWSTR strIn);

		operator std::string() const { return str; }
		operator const char*() const { return str.c_str(); }

		const WCHAR * UnicodeString::GetUnicodePtr() const { return wstr.c_str(); }
		const char * UnicodeString::GetASCIIPtr() const { return str.c_str(); }
	};
}


float inline lerp(float low, float high, float t) { return low + (high - low) * t; }

/// RANDOM
//===============================================================================================
float	RandF(float l, float h);
int		RandI(int l, int h);
size_t	RandU(size_t l, size_t h);

#endif