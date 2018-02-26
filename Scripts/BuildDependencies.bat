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
