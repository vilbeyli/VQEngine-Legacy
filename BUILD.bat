@echo off
REM 	DX11Renderer - VDemo | DirectX11 Renderer
REM 	Copyright(C) 2018  - Volkan Ilbeyli
REM 
REM 	This program is free software : you can redistribute it and / or modify
REM 	it under the terms of the GNU General Public License as published by
REM 	the Free Software Foundation, either version 3 of the License, or
REM 	(at your option) any later version.
REM 
REM 	This program is distributed in the hope that it will be useful,
REM 	but WITHOUT ANY WARRANTY; without even the implied warranty of
REM 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
REM 	GNU General Public License for more details.
REM 
REM 	You should have received a copy of the GNU General Public License
REM 	along with this program.If not, see <http://www.gnu.org/licenses/>.
REM 
REM 	Contact: volkanilbeyli@gmail.com

REM --------------------------------------------------------------------------------------
REM Notes:
REM  - define variables with set, reference them by enclosing variables between %
REM  - read arguments with %1, %2, %3...
REM  - setting numeric variables with /A flag: SET /A a=5
REM  - check process return code with %ERRORLEVEL%
REM 
REM --------------------------------------------------------------------------------------

REM setlocal so that the variables defined here doesn't live through the lifetime
REM of this script file. the calling cmd.exe would still see them if we don't define
REM the variables in a setlocal-endlocal scope
setlocal

rem TODO: define a function - read registry - return devenv path
rem https://developercommunity.visualstudio.com/content/problem/2813/cant-find-registry-entries-for-visual-studio-2017.html
set devenv=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.exe
set msbuild=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe

set solution=VQEngine.sln

set log_devenv=%APPDATA%\VQEngine\Logs\Build\devenv_out.log
set log_msbuild_s=%APPDATA%\VQEngine\Logs\Build\msbuild_out_st.log
set log_msbuild_m=%APPDATA%\VQEngine\Logs\Build\msbuild_out_mt.log

REM TODO: figure out multi-threaded msbuild or faster. this takes 42s while devenv takes ~20s
REM set MSBuild_MultiThreaded=True
REM call :CleanUp
REM call :MSBuild_Build %MSBuild_MultiThreaded% %log_msbuild_m%

REM set MSBuild_MultiThreaded=False
REM call :CleanUp
REM call :MSBuild_Build %MSBuild_MultiThreaded% %log_msbuild_s%

cls
if not exist "%APPDATA%\VQEngine\Logs\Build" mkdir "%APPDATA%\VQEngine\Logs\Build

call :CleanUp
call :Devenv_Build RELEASE, %log_devenv%
call :PackageBinaries

REM Error checking
if %ERRORLEVEL% GEQ 1 (
    echo Error building solution file: %solution%
    pause
)

@echo:
echo Build Finished.
@echo:
echo ./Build/_artifacts contains the release version of the project executable.

endlocal
EXIT /B 0


REM Cleanup before build for a clean build
:CleanUp
echo Cleaning up the project folders...
if not exist Build mkdir Build
cd Build
if not exist Empty mkdir Empty
robocopy ./Empty ./ /purge /MT:8 > nul
cd ..
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
REM get all .dll and .exe files from build
for /r "%proj_dir%" %%f in (*.exe *.dll) do (
    REM skip _artifacts folder itself
    if not %%~df%%~pf == %artifacts_dir%\ (  
        xcopy %%f %artifacts_dir% > nul
    )
) 
REM copy data and shaders
robocopy ./Data %artifacts_dir%/Data /E /MT:8 > nul
robocopy ./Source/Shaders %artifacts_dir%/Source/Shaders /E /MT:8 > nul
EXIT /B 0


:MSBuild_Build
if %1==True (Building the project [MSBuild Multi-thread]...) else (echo Building the project [MSBuild Single-thread]...)

REM cleanup log file if it exists
if exist %2 del %2 

set t_msb_begin=%TIME%
echo              begin %t_msb_begin%
"%msbuild%" %solution%  /p:Configuration=Release /p:BuildInParallel=%1 > %2
set t_msb_end=%TIME%
echo              end   %t_msb_end%
rem set duration=%t_msb_end%-%t_msb_begin%
echo ------------------------------------------
EXIT /B 0

:Devenv_Build

REM cleanup log file if it exists
if exist %2 del %2 

echo Building the project [devenv]...
echo              begin %TIME%
"%devenv%" %solution% /build %1 /out %2
echo              end   %TIME%
echo ------------------------------------------
REM TODO: read file line by line, cat crashes with stackdump...
REM cat %logFile%
EXIT /B 0


