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

set /A ALL_SUBMODULES_INITIALIZED=0

call :IsEmpty %FREETYPE%
set /A ALL_SUBMODULES_INITIALIZED=%ALL_SUBMODULES_INITIALIZED% + %errorlevel%

call :IsEmpty %DIRECTXTEX%
set /A ALL_SUBMODULES_INITIALIZED=%ALL_SUBMODULES_INITIALIZED% + %errorlevel%

if %ALL_SUBMODULES_INITIALIZED% NEQ 0 (
  start /w git submodule update --init --recursive
)

exit /b 0



:InitSubmodule
if not "%1" == "" (
    call :IsEmpty %1 
    if %errorlevel%==0 (
        echo %1 empty
        pushd %1
        ::call git submodule update --init --recursive
        start /w git submodule update --init --recursive
        popd
    )
    else (
        echo %1 non-empty
    )
) else (
  echo InitSubmodule: error - no parameter
)
exit /b 0


:IsEmpty
if not "%1" == "" (
  if exist "%1" (
    ( dir /b /a "%1" | findstr . ) > nul && (
      exit /b 0
    ) || (
      exit /b 1
    )
  ) else (
    echo %1 missing
  )
) else (
  echo IsEmpty: error - no parameter
)
goto :EOF
