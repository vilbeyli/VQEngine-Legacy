@echo off
setlocal enabledelayedexpansion

set GFX_API=
set SOLUTION_FILE_BASE=VQEngine_
set SOLUTION_FILE=
set LAUNCH_VS=0

for %%i IN (%*) DO (
    if "%%i"=="-noVS" (
        set LAUNCH_VS=0
    )


)
:: Check arguments and set $GFX_API
if "%~1"=="" (
    echo Default API is DirectX11
    call :SetGFX_API_DX11
) else (
    if "%~1"=="DX11"  call :SetGFX_API_DX11
    if "%~1"=="-DX11" call :SetGFX_API_DX11
    if "%~1"=="/DX11" call :SetGFX_API_DX11

    if "%~1"=="DirectX11"  call :SetGFX_API_DX11
    if "%~1"=="-DirectX11" call :SetGFX_API_DX11
    if "%~1"=="/DirectX11" call :SetGFX_API_DX11

    if "%~1"=="DX12"  call :SetGFX_API_DX12
    if "%~1"=="-DX12" call :SetGFX_API_DX12
    if "%~1"=="/DX12" call :SetGFX_API_DX12

    if "%~1"=="DirectX12"  call :SetGFX_API_DX12
    if "%~1"=="-DirectX12" call :SetGFX_API_DX12
    if "%~1"=="/DirectX12" call :SetGFX_API_DX12

    if "%~1"=="DX12"  call :SetGFX_API_DX12
    if "%~1"=="-DX12" call :SetGFX_API_DX12
    if "%~1"=="/DX12" call :SetGFX_API_DX12
    
    if "%~1"=="help" call :PrintUsage
    if "%~1"=="-h"   call :PrintUsage
    if "%~1"=="/h"   call :PrintUsage
)

:: Check if GFX_API is set & build SOLUTION_FILE name
if "!GFX_API!"=="" (
    echo Incorrect parameter: %~1
    call :PrintUsage
    exit /b -1
)

::set SOLUTION_FILE=%SOLUTION_FILE_BASE%!GFX_API!.sln
set SOLUTION_FILE=VQEngine.sln

cls
echo Checking pre-requisites...

:: Check if CMake is installed
cmake --version > nul 2>&1
if %errorlevel% NEQ 0 (
    echo Cannot find path to cmake. Is CMake installed? Exiting...
    exit /b -1
) else (
    echo    CMake      - Ready.
)

:: Check if submodule is initialized to avoid CMake file not found errors
set SUBMODULE_DIR=..\Source\3rdParty\DirectXTex\
set SUBMODULE_FILE=!SUBMODULE_DIR!DirectXTex_Desktop_2017.sln
if not exist !SUBMODULE_FILE! (
    echo    Git Submodules   - Not Ready: File common.cmake doesn't exist in '!SUBMODULE_DIR!'  -  Initializing submodule...

    :: attempt to initialize submodule
    cd ..
    echo.
    git submodule init
    git submodule update
    cd build

    :: check if submodule initialized properly
    if not exist !SUBMODULE_FILE! (
        echo.
        echo Could not initialize submodule. Make sure all the submodules are initialized and updated.
        echo Exiting...
        echo.
        exit /b -1 
    ) else (
        echo    Git Submodules   - Ready.
    )
) else (
    echo    Git Submodules   - Ready.
)




echo.
echo Generating solution files...

:: Generate Build directory
if not exist !GFX_API! (
    echo Creating directory !GFX_API!...
    mkdir !GFX_API!
)

:: Run CMake
cd !GFX_API!
cmake ..\.. -DGFX_API=!GFX_API!


if %errorlevel% EQU 0 (
    echo Success!
    if !LAUNCH_VS! EQU 1 (
        start %SOLUTION_FILE%
    )
) else (
    echo.
    echo GenerateSolutions.bat: Error with CMake. No solution file generated. 
    echo.
    pause
)

cd ..
exit /b 0


:SetGFX_API_DX11
set GFX_API=DX11
exit /b 0

:SetGFX_API_DX12
set GFX_API=DX12
exit /b 0

:PrintUsage
echo Usage    : GenerateSolutions.bat [API]
echo.
echo Examples : GenerateSolutions.bat VK
echo            GenerateSolutions.bat DX
echo            GenerateSolutions.bat Vulkan
echo            GenerateSolutions.bat DX12
exit /b 0f