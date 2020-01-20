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

set FREETYPE="Source\3rdParty\freetype-windows-binaries"
set DIRECTXTEX="Source\3rdParty\DirectXTex"

echo Cleaning up submodule folders...

call :Clean %FREETYPE%
call :Clean %DIRECTXTEX%

exit /b 0



:Clean
if not exist %1 mkdir %1
pushd %1
if not exist Empty mkdir Empty
robocopy ./Empty ./ /purge /MT:8 > nul
popd