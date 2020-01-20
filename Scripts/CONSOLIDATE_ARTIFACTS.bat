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