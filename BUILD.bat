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
set devenv=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.exe
set msbuild=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe

set solution=VDemo.sln

set log_devenv=./LogFiles/devenv_out.log
set log_msbuild_s=./LogFiles/msbuild_out_st.log
set log_msbuild_m=./LogFiles/msbuild_out_mt.log

REM TODO: figure out multi-threaded msbuild or faster. this takes 42s while devenv takes ~20s
REM set MSBuild_MultiThreaded=True
REM call :CleanUp
REM call :MSBuild_Build %MSBuild_MultiThreaded% %log_msbuild_m%

REM set MSBuild_MultiThreaded=False
REM call :CleanUp
REM call :MSBuild_Build %MSBuild_MultiThreaded% %log_msbuild_s%

call :CleanUp
call :Devenv_Build RELEASE, %log_devenv%

REM Error checking
if %ERRORLEVEL% GEQ 1 echo Error building solution file: %solution%

endlocal
pause
EXIT /B 0


REM Cleanup before build for a clean build
:CleanUp
echo Robocopy   : Cleaning up the project folders...
cd Build
mkdir Empty
robocopy ./Empty ./ /purge /MT:8 > nul
cd ..
EXIT /B 0

:MSBuild_Build
if %1==True (echo MSBuild    : Building the project [Multi-thread]...) else (echo MSBuild    : Building the project [Single-thread]...)

REM cleanup log file if it exists
if exist %2 rm %2 

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
if exist %2 rm %2 

echo devenv     : Building the project...
echo              begin %TIME%
"%devenv%" %solution% /build %1 /out %2
echo              end   %TIME%
echo ------------------------------------------
REM TODO: read file line by line, cat crashes with stackdump...
REM cat %logFile%
EXIT /B 0


