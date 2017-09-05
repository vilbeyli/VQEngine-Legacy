
@echo off
REM Turn echo off - no spamming

REM 	DX11Renderer - VDemo | DirectX11 Renderer
REM 	Copyright(C) 2016  - Volkan Ilbeyli
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


REM DEPENDENCY BUILDING IS HANDLED BY VISUAL STUDIO AS PRE-BUILD EVENT WITH THE FOLLOWING SCRIPT
SET DXTexture_proj_location=$(SolutionDir)Source\3rdParty\DirectXTex
IF EXIST $(ProjectDir)..\3rdParty\DirectXTex\DirectXTex\Bin\Desktop_2015\$(Platform)\$(Configuration)\DirectXTex.lib (
echo DirectXTex.lib already built. ) ELSE (
echo Building dependency library: DirectXTex... 
devenv %DXTexture_proj_location%\DirectXTex_Desktop_2015.sln /Build "$(Configuration)|$(Platform)")
REM DEPENDENCY BUILDING IS HANDLED BY VISUAL STUDIO AS PRE-BUILD EVENT WITH THE ABOVE SCRIPT

REM echo %0
REM echo %CD%

REM -------------------------------------
REM ----- DEPENDENCY LOCATIONS      -----
REM -------------------------------------
REM Dependency : DirectXTex

rem SET loc_DXTex ; by visual studio
rem SET orig_loc=%CD%


REM TODO: 
REM - check if project is built
REM - build debug/release x86/x64 whicever's missing | C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin
REM - make this pre-build script for VDemo.sln


REM -------------------------------------
REM ----- COMPILE DEPENDENCIES      -----
REM -------------------------------------
REM goto dependency location
cd %DXTexture_Proj_location%
echo %CD%

REM $(ProjectDir)..\3rdParty\DirectXTex\DirectXTex\Bin\Desktop_2015\$(Platform)\$(Configuration)

REM echo current directory 
REM call BuildDirecXTex_Desktop2015.bat
echo HELLOOOOOOOOOOOOOOO

REM return back to original directory
rem cd %orig_loc%

REM pause to show results
REM pause
