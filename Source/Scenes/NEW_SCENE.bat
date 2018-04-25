@echo off
REM 	VQEngine | DirectX11 Renderer
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

setlocal

REM Set xcopy parameters for generating scene files
set tmpFileH=.\SceneTemplate.h
set tmpFileCpp=.\SceneTemplate.cpp

if %1.==. (
    set flags=/Y
    set fileNameCpp=.\NewScene.cpp
    set fileNameH=.\NewScene.h
    set fileName=NewScene
) else (
    set flags=
    set fileNameCpp=.\%1.cpp
    set fileNameH=.\%1.h
    set fileName=%1
)

REM src: https://stackoverflow.com/questions/60034/how-can-you-find-and-replace-text-in-a-file-using-the-windows-command-line-envir
REM Generate scene files, replace Template name with scene file name
set stringFind=SceneTemplate
set stringReplace=%fileName%

powershell -Command "(gc %tmpFileCpp%) -replace '%stringFind%', '%stringReplace%' | Out-File -encoding ASCII %fileNameCpp%"
powershell -Command "(gc %tmpFileH%) -replace '%stringFind%', '%stringReplace%' | Out-File -encoding ASCII %fileNameH%"


REM src: https://stackoverflow.com/questions/7005951/batch-file-find-if-substring-is-in-string-not-in-a-file
REM src: https://stackoverflow.com/questions/23525681/how-to-print-variable-inside-loop-in-batch-script
REM Edit solution files and add the new scene files to the project file
set "includeXML=ClInclude Include="$(SolutionDir)Source\Scenes\%fileName%.h" /"
set "compileXML=ClCompile Include="$(SolutionDir)Source\Scenes\%fileName%.cpp" /"
set "prevSearchStrCompile=ClCompile Include="""
set "prevSearchStrInclude=ClInclude Include="""
set "currSearchStr=/ItemGroup"
set "solutionFile=Scenes.vcxproj"
set "updatedSolutionOutDir=.\new\Scenes.vcxproj"

cd ..\SolutionFiles
mkdir new

@setlocal enableextensions enabledelayedexpansion
@echo off
REM find the lines with CLInclude and CLCompile (=prevLine), which is also followed by </ItemGroup>
REM </ItemGroup> as current line and CLInclude/Compile as prevLine, we output to the new solution file
set prevLine=
for /f "tokens=* delims=," %%a in (%solutionFile%) do (
  set line=%%a
  if not "x!line:%currSearchStr%=!"=="x!line!" (
       if not "x!prevLine:%prevSearchStrCompile%=!"=="x!prevLine!" (
           REM !line! currently has </ItemGroup>, !prevLine! currently has the last cpp entry
           echo     ^<%compileXML%^> >> %updatedSolutionOutDir%
       )
       if not "x!prevLine:%prevSearchStrInclude%=!"=="x!prevLine!" (
           REM !line! currently has </ItemGroup>, !prevLine! currently has the last include entry
           echo     ^<%includeXML%^> >> %updatedSolutionOutDir%
       )
  )
  echo %%a>> %updatedSolutionOutDir%
  set prevLine=!line!
)
endlocal

xcopy /Y %updatedSolutionOutDir% %solutionFile%*
del %updatedSolutionOutDir%
rmdir new
cd ..\Scenes
