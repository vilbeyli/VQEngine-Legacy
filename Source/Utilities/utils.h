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

#include <string>
#include <vector>

#define RANGE(c) std::begin(c), std::end(c)
#define RRANGE(c) std::rbegin(c), std::rend(c)



namespace StrUtil
{
	// typedefs
	typedef wchar_t WCHAR;    // wc,   16-bit UNICODE character
	typedef wchar_t* PWSTR;


	std::vector<std::string> split(const char* s, char c = ' ');
	std::vector<std::string> split(const std::string& s, char c = ' ');
	std::vector<std::string> split(const std::string& s, const std::vector<char>& delimiters);
	template<class... Args> std::vector<std::string> split(const std::string& s, Args&&... args)
	{
		const std::vector<char> delimiters = { args... };
		return split(s, delimiters);
	}

	std::string CommaSeparatedNumber(const std::string& num);

	struct UnicodeString
	{
	public:
		static std::string ToASCII(PWSTR pwstr) { std::wstring wstr(pwstr); return std::string(wstr.begin(), wstr.end()); }
		static std::wstring ASCIIToUnicode(const std::string& str) { return std::wstring(str.begin(), str.end()); }

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



namespace DirectoryUtil
{
	enum ESpecialFolder
	{
		PROGRAM_FILES,
		APPDATA,
		LOCALAPPDATA,
		USERPROFILE,
	};

	std::string	GetSpecialFolderPath(ESpecialFolder folder);
	std::string	GetFileNameWithoutExtension(const std::string&);
	std::string	GetFileNameFromPath(const std::string& filePath);
	std::string GetFileExtension(const std::string& filePath);
	bool		FileExists(const std::string& pathToFile);

	// returns true if folder exists, false otherwise after creating the folder
	//
	bool		CreateFolderIfItDoesntExist(const std::string& directoryPath);

	// returns the folder path given a file path
	// e.g.: @pathToFile="C:\\Folder\\File.txt" -> returns "C:\\Folder\\"
	//
	std::string GetFolderPath(const std::string& pathToFile);

	// returns true if the given @pathToImageFile ends with .jpg, .png or .hdr
	//
	bool		IsImageName(const std::string& pathToImageFile);

	// returns true if @file0 has been written into later than @file1 has.
	//
	bool		IsFileNewer(const std::string& file0, const std::string& file1);
}


// returns current time in format "YYYY-MM-DD_HH-MM-SS"
std::string GetCurrentTimeAsString();
std::string GetCurrentTimeAsStringWithBrackets();




namespace MathUtil
{
	template<class T> inline T lerp(T low, T high, float t) { return low + static_cast<T>((high - low) * t); }
	template<class T> void ClampedIncrementOrDecrement(T& val, int upOrDown, int lo, int hi)
	{
		int newVal = static_cast<int>(val) + upOrDown;
		if (upOrDown > 0) newVal = newVal >= hi ? (hi - 1) : newVal;
		else              newVal = newVal < lo ? lo : newVal;
		val = static_cast<T>(newVal);
	}
	template<class T> void Clamp(T& val, const T lo, const T hi)
	{
		val = min(val, lo);
		val = max(val, hi);
	}
	template<class T> T Clamp(const T& val, const T lo, const T hi)
	{
		T _val = val;
		Clamp(_val, lo, hi);
		return _val;
	}

	float	RandF(float l, float h);
	int		RandI(int l, int h);
	size_t	RandU(size_t l, size_t h);
}


#include "Renderer/RenderingEnums.h"
std::string ImageFormatToFileExtension(const EImageFormat format);


