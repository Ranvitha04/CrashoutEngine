@echo off
setlocal
pushd "%~dp0"

set "BUILD_DIR=build"
set "EXE_NAME=Descent.exe"

REM --- Configuration Selection for Building and Running ---
set "CONFIG=%1"
if not defined CONFIG (
    set "CONFIG=Debug"
)
REM Normalize case
if /i "%CONFIG%"=="debug" set "CONFIG=Debug"
if /i "%CONFIG%"=="release" set "CONFIG=Release"

echo Target Configuration: %CONFIG%

REM STEP 1: Configure (fast, incremental)
echo [1/3] Configuring project with CMake...
cmake -S . -B "%BUILD_DIR%"
if ERRORLEVEL 1 goto :FAIL

REM STEP 2: Build (only the selected configuration)
echo [2/3] Building %CONFIG% configuration...
cmake --build "%BUILD_DIR%" --config %CONFIG%
if ERRORLEVEL 1 goto :FAIL

REM STEP 3: Run the executable
echo [3/3] Launching executable...
set "EXE_PATH=%BUILD_DIR%\%CONFIG%\%EXE_NAME%"

if exist "%EXE_PATH%" (
    echo Found: %EXE_PATH%
    
    REM Get the directory where the .exe is located
    for %%F in ("%EXE_PATH%") do set "EXE_DIR=%%~dpF"

    REM Use /D to set the correct working directory BEFORE launching
    REM This is the most reliable method.
    start "" /D "%EXE_DIR%" "%EXE_PATH%"
) else (
    echo ERROR: Executable not found at %EXE_PATH%
    goto :FAIL
)

echo.
echo --- SUCCESS ---
goto :END

:FAIL
echo.
echo --- FAILED ---
echo An error occurred. Please check the messages above.

:END
popd
endlocal
pause