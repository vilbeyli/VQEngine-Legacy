:: 	DX11Renderer - VDemo | DirectX11 Renderer
:: 	Copyright(C) 2018  - Volkan Ilbeyli
:: 
:: 	This program is free software : you can redistribute it and / or modify
:: 	it under the terms of the GNU General Public License as published by
:: 	the Free Software Foundation, either version 3 of the License, or
:: 	(at your option) any later version.
:: 
:: 	This program is distributed in the hope that it will be useful,
:: 	but WITHOUT ANY WARRANTY; without even the implied warranty of
:: 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
:: 	GNU General Public License for more details.
:: 
:: 	You should have received a copy of the GNU General Public License
:: 	along with this program.If not, see <http://www.gnu.org/licenses/>.
:: 
:: 	Contact: volkanilbeyli@gmail.com
@echo off
setlocal enabledelayedexpansion

set VSWHERE="%PROGRAMFILES(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set MSBUILD_QUERY=%VSWHERE% -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe
set MSBUILD=
set API=
set SOLUTION_DIRECTORY=
set SOLUTION_FILE=
set BUILD_CONFIGURATION=
set OPEN_BINARY_OUTPUT_DIR=0
set BINARY_OUTPUT_DIR=..\bin\
set FORCE_BUILD=0
set CLEAN_BEFORE_BUILD=0

set FILE_TO_CHECK_FOR_SUBMODULE_INITIALIZATION=../Source/3rdParty/DirectXTex/DirectXTex/DirectXTex_Desktop_2017.vcxproj
set FILE_TO_CHECK_FOR_PROJECT_BUILT=../Source/3rdParty/DirectXTex/DirectXTex/Bin/Desktop_2017/x64/

::
:: Parameter Scan
::
for %%i IN (%*) DO (
    if "%%i"=="-Release" set BUILD_CONFIGURATION=Release
    if "%%i"=="-R"       set BUILD_CONFIGURATION=Release
    
    if "%%i"=="-Debug"   set BUILD_CONFIGURATION=Debug
    if "%%i"=="-D"       set BUILD_CONFIGURATION=Debug

    if "%%i"=="-F"       set FORCE_BUILD=1
    if "%%i"=="-f"       set FORCE_BUILD=1

    if "%%i"=="-Clean"   set CLEAN_BEFORE_BUILD=1
    if "%%i"=="-C"       set CLEAN_BEFORE_BUILD=1
)

::
:: Main
::
call :CHECK_SUBMODULE_INITIALIZED
if !ERRORLEVEL! NEQ 0 (
    echo Error : Submodule not initialized.
    exit /b -1
)

call :FIND_MSBUILD
if !ERRORLEVEL! NEQ 0 (
    echo Error : Cannot find MSBuild.exe
    exit /b -1
)

set SOLUTION_FOLDER=../Source/3rdParty/DirectXTex/DirectXTex/
set SOLUTION_FILE=DirectXTex_Desktop_2017.vcxproj
if "!BUILD_CONFIGURATION!"=="" (
    set BUILD_CONFIGURATION=Release
)

set FILE_TO_CHECK_FOR_PROJECT_BUILT=%~dp0!FILE_TO_CHECK_FOR_PROJECT_BUILT!!BUILD_CONFIGURATION!/DirectXTex.lib
if not exist !FILE_TO_CHECK_FOR_PROJECT_BUILT! (
    echo FILE_TO_CHECK_FOR_PROJECT_BUILT doesn't exist: !FILE_TO_CHECK_FOR_PROJECT_BUILT!
    call :BUILD_PROJECT_FILE
    exit /b 0
) else (
    echo "DirectXTex<!BUILD_CONFIGURATION!> is already built."
)

if !FORCE_BUILD! EQU 1 (
    call :BUILD_PROJECT_FILE
)

exit /b 0


::
:: BUILD_PROJECT_FILE()
::
:BUILD_PROJECT_FILE
set SOLUTION_DIRECTORY=%~dp0!SOLUTION_FOLDER!!SOLUTION_FILE!
if exist !SOLUTION_DIRECTORY! (
    echo.
    echo MSBuild Location: !MSBUILD!
    echo Building project !SOLUTION_DIRECTORY! for !BUILD_CONFIGURATION! configuration...
    echo.
    if !CLEAN_BEFORE_BUILD! EQU 1 (
        set BUILD_FLAGS=/p:Configuration=!BUILD_CONFIGURATION! /p:Platform=x64 /t:Clean
        "!MSBUILD!" "!SOLUTION_DIRECTORY!" !BUILD_FLAGS!
    )
    set BUILD_FLAGS=/p:Configuration=!BUILD_CONFIGURATION! /p:Platform=x64
    "!MSBUILD!" "!SOLUTION_DIRECTORY!" !BUILD_FLAGS!
) else (
    echo Error: Solution file could not be found: !SOLUTION_DIRECTORY!
    echo.
    exit /b -1
)
exit /b 0



::
:: CHECK_SUBMODULE_INITIALIZED()
::
:CHECK_SUBMODULE_INITIALIZED
set FILE_TO_CHECK_FOR_SUBMODULE_INITIALIZATION="%~dp0"!FILE_TO_CHECK_FOR_SUBMODULE_INITIALIZATION!
if exist !FILE_TO_CHECK_FOR_SUBMODULE_INITIALIZATION! (
    exit /b 0
) else (
    echo !FILE_TO_CHECK_FOR_SUBMODULE_INITIALIZATION! , CD=!cd!
    echo also: DIR_TEST=!DIR_TEST!
    echo Submodule DirectXTex not initialized.
    echo Attempting to initialize git submodules...
    git submodule init
    git submodule update
    if exist !FILE_TO_CHECK_FOR_SUBMODULE_INITIALIZATION! (
        exit /b 0
    )
)
exit /b -1


::
:: FIND_MSBUILD(): Finds MSBuild.exe's location and sets the %MSBUILD% variable
::
:FIND_MSBUILD
for /f "usebackq tokens=*" %%i IN (`%MSBUILD_QUERY%`) DO (
    set MSBUILD=%%i
    goto CHECK_MSBUILD
)
:CHECK_MSBUILD
if not exist !MSBUILD! (
    echo Build Error: MSBuild.exe could not be located.
    echo.
    exit /b -1
)
if "%~1"=="true" (
    echo MSBuild Found: !MSBUILD!
)
exit /b 0
