
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

REM echo %0
REM echo %CD%

REM -------------------------------------
REM ----- DEPENDENCY LOCATIONS      -----
REM -------------------------------------
REM Dependency : DirectXTex
SET loc_DXTex=Source\3rdParty\DirectXTex
SET orig_loc=%CD%


REM TODO: 
REM - check if project is built
REM - build debug/release x86/x64 whicever's missing | C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin
REM - make this pre-build script for VDemo.sln


REM -------------------------------------
REM ----- COMPILE DEPENDENCIES      -----
REM -------------------------------------
REM goto dependency location
cd ../%loc_DXTex%

REM echo current directory 
call BuildDirecXTex_Desktop2015.bat

REM return back to original directory
cd %orig_loc%

REM pause to show results
REM pause