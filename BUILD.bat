@echo off
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

:: --------------------------------------------------------------------------------------
:: Notes:
::  - define variables with set, reference them by enclosing variables between %
::  - read arguments with %1, %2, %3...
::  - setting numeric variables with /A flag: SET /A a=5
::  - check process return code with %ERRORLEVEL%
:: 
:: --------------------------------------------------------------------------------------

:: setlocal so that the variables defined here doesn't live through the lifetime
:: of this script file. the calling cmd.exe would still see them if we don't define
:: the variables in a setlocal-endlocal scope
setlocal

:: TODO: define a function - read registry - return devenv path
:: https://developercommunity.visualstudio.com/content/problem/2813/cant-find-registry-entries-for-visual-studio-2017.html
set devenv=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.exe
set msbuild=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe

set solution=VQEngine.sln

set log_devenv=%APPDATA%\VQEngine\Logs\Build\devenv_out.log
set log_msbuild_s=%APPDATA%\VQEngine\Logs\Build\msbuild_out_st.log
set log_msbuild_m=%APPDATA%\VQEngine\Logs\Build\msbuild_out_mt.log

:: TODO: figure out multi-threaded msbuild or faster. this takes 42s while devenv takes ~20s
:: set MSBuild_MultiThreaded=True
:: call :CleanUp
:: call :MSBuild_Build %MSBuild_MultiThreaded% %log_msbuild_m%

:: set MSBuild_MultiThreaded=False
:: call :CleanUp
:: call :MSBuild_Build %MSBuild_MultiThreaded% %log_msbuild_s%

cls
if not exist "%APPDATA%\VQEngine\Logs\Build" mkdir "%APPDATA%\VQEngine\Logs\Build

call :MSBuild_Build RELEASE, %log_devenv%
call :PackageBinaries

:: Error checking
if %ERRORLEVEL% GEQ 1 (
    echo Error building solution file: %solution%
    pause
)

@echo:
echo Build Finished.
@echo:
echo ./Build/_artifacts contains the release version of the project executable.

:: open final directory w/ explorer
::~dp0 is the directory this batch file is in
start explorer "%~dp0Build\_artifacts"

pause
endlocal
EXIT /B 0



:PackageBinaries
echo Cleaning up the artifacts folder...
cd Build
if not exist Empty mkdir Empty
robocopy ./Empty ./_artifacts /purge /MT:8 > nul
set proj_dir=%cd%
set artifacts_dir=%proj_dir%\_artifacts
rmdir Empty
cd ..
set root_dir=%cd%
:: get all .dll and .exe files from build
for /r "%proj_dir%" %%f in (*.exe *.dll) do (
    :: skip _artifacts folder itself
    if not "%%~df%%~pf" == "%artifacts_dir%\" ( 
        xcopy "%%f" "%artifacts_dir%" /Y > nul
    )
) 
:: copy data and shaders
robocopy ./Data "%artifacts_dir%/Data" /E /MT:8 > nul
robocopy ./Source/Shaders "%artifacts_dir%/Source/Shaders" /E /MT:8 > nul

:: EngineSettings.ini ends up in repo root after Build.
if exist EngineSettings.ini (
    xcopy "EngineSettings.ini" "./Build/_artifacts/"\ /Y /Q /F /MOV
) else (
    xcopy "./Data/EngineSettings.ini" "./Build/_artifacts/"\ /Y /Q /F
)

if %errorlevel% NEQ 0 (
    xcopy "./Data/EngineSettings.ini" "./Build/_artifacts/"\ /Y /Q /F
)


EXIT /B 0


:MSBuild_Build
if %1==True (Building the project [MSBuild Multi-thread]...) else (echo Building the project [MSBuild Single-thread]...)

:: cleanup log file if it exists
if exist %2 del %2 

set t_msb_begin=%TIME%
echo              begin %t_msb_begin%
"%msbuild%" %solution%  /p:Configuration=Release /m
set t_msb_end=%TIME%
echo              end   %t_msb_end%
:: set duration=%t_msb_end%-%t_msb_begin%
echo ------------------------------------------
EXIT /B 0

:Devenv_Build

:: cleanup log file if it exists
if exist %2 del %2 

echo Building the project [devenv]...
echo              begin %TIME%
"%devenv%" %solution% /build %1 /out %2
echo              end   %TIME%
echo ------------------------------------------
:: TODO: read file line by line, cat crashes with stackdump...
:: cat %logFile%
EXIT /B 0


