@echo off
setlocal enabledelayedexpansion
pushd "%~dp0"

set "EXE_NAME=Descent.exe"
set "SLN_FILE=CrashoutEngine.sln"
set "BUILD_DIR=build"

REM --- Configuration Selection ---
set "CONFIG=%~1"
if not defined CONFIG set "CONFIG=Debug"
if /i "%CONFIG%"=="debug" set "CONFIG=Debug"
if /i "%CONFIG%"=="release" set "CONFIG=Release"

REM --- Start timer ---
set "start=%time%"
echo Target Configuration: %CONFIG%

REM --- Ensure build folder exists ---
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM STEP 1: Configure (only if needed)
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo [1/3] Configuring project with CMake...
    cmake -S . -B "%BUILD_DIR%" -A x64
    if ERRORLEVEL 1 goto :FAIL
) else (
    echo [1/3] Skipping configure (already configured)
)

REM --- Auto-open Visual Studio Solution early ---
set "SLN_PATH=%BUILD_DIR%\%SLN_FILE%"
if exist "!SLN_PATH!" (
    echo Opening Visual Studio: !SLN_PATH!
    start "" "!SLN_PATH!"
) else (
    echo Solution not found yet. It will open after configure if available.
)

REM STEP 2: Build (runs while VS opens)
echo [2/3] Building %CONFIG% configuration...
cmake --build "%BUILD_DIR%" --config %CONFIG% --parallel
if ERRORLEVEL 1 goto :FAIL

REM STEP 3: Run executable
echo [3/3] Launching executable...
set "EXE_PATH=%BUILD_DIR%\x64-%CONFIG%\%EXE_NAME%"
if exist "%EXE_PATH%" (
    echo Found: %EXE_PATH%
    start "" /D "%BUILD_DIR%\x64-%CONFIG%" "%EXE_NAME%"
) else (
    echo ERROR: Executable not found at %EXE_PATH%
    goto :FAIL
)

echo.
echo --- SUCCESS ---
goto :TIMER

:FAIL
echo.
echo --- FAILED ---
echo An error occurred. Please check the messages above.

:TIMER
REM --- End timer and compute duration ---
set "end=%time%"
for /f "tokens=1-4 delims=:." %%a in ("%start%") do (
    set /a "startms=(1%%a-100)*3600000 + (1%%b-100)*60000 + (1%%c-100)*1000 + 1%%d-100"
)
for /f "tokens=1-4 delims=:." %%a in ("%end%") do (
    set /a "endms=(1%%a-100)*3600000 + (1%%b-100)*60000 + (1%%c-100)*1000 + 1%%d-100"
)
if !endms! lss !startms! set /a endms+=86400000

set /a elapsed=!endms!-!startms!
set /a minutes=!elapsed!/60000
set /a seconds=(!elapsed!%%60000)/1000
echo.
echo Total Runtime: !minutes! min !seconds! sec
echo.

popd
endlocal
pause
