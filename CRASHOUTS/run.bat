@echo off
setlocal enabledelayedexpansion
pushd "%~dp0"

REM ======================================================
REM Default settings
REM ======================================================
set "DEFAULT_TARGET=CrashoutEditor"
set "DEFAULT_CONFIG=Debug"
REM Capture start time
set "start=%time%"
REM ======================================================
REM Parse arguments
REM Usage:
REM   run.bat                → CrashoutEditor Debug
REM   run.bat Release        → CrashoutEditor Release
REM   run.bat Descent        → Descent Debug
REM   run.bat Descent Release → Descent Release
REM ======================================================

set "TARGET_INPUT=%~1"
set "CONFIG_INPUT=%~2"

REM Determine target
if /i "%TARGET_INPUT%"=="Descent" (
    set "TARGET=Descent"
) else if /i "%TARGET_INPUT%"=="CrashoutEditor" (
    set "TARGET=CrashoutEditor"
) else if /i "%TARGET_INPUT%"=="Debug" (
    set "TARGET=%DEFAULT_TARGET%"
    set "CONFIG=Debug"
) else if /i "%TARGET_INPUT%"=="Release" (
    set "TARGET=%DEFAULT_TARGET%"
    set "CONFIG=Release"
) else if "%TARGET_INPUT%"=="" (
    set "TARGET=%DEFAULT_TARGET%"
) else (
    REM Unknown input → treat as default target
    set "TARGET=%DEFAULT_TARGET%"
)

REM Determine config if not already set
if not defined CONFIG (
    if /i "%CONFIG_INPUT%"=="Release" (
        set "CONFIG=Release"
    ) else if /i "%CONFIG_INPUT%"=="Debug" (
        set "CONFIG=Debug"
    ) else (
        set "CONFIG=%DEFAULT_CONFIG%"
    )
)

echo.
echo ======================================================
echo   Building Target: %TARGET%
echo   Configuration : %CONFIG%
echo ======================================================
echo.

REM ======================================================
REM Build folder
REM ======================================================
set "BUILD_DIR=build"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM ======================================================
REM Configure (only once)
REM ======================================================
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo [1/3] Configuring project with CMake...
    cmake -S . -B "%BUILD_DIR%" -A x64
    if ERRORLEVEL 1 goto :FAIL
) else (
    echo [1/3] Skipping configure (already configured)
)

REM ======================================================
REM Auto-open solution
REM ======================================================
set "SLN_PATH=%BUILD_DIR%\CrashoutEngine.sln"
if exist "%SLN_PATH%" (
    echo Opening Visual Studio solution...
    start "" "%SLN_PATH%"
)

REM ======================================================
REM Build only the requested target
REM ======================================================
echo [2/3] Building %TARGET% (%CONFIG%)...
cmake --build "%BUILD_DIR%" --target %TARGET% --config %CONFIG% --parallel -j 8
if ERRORLEVEL 1 goto :FAIL

REM ======================================================
REM Determine executable name
REM ======================================================
if /i "%TARGET%"=="Descent" (
    set "EXE_NAME=Descent.exe"
) else (
    set "EXE_NAME=CrashoutEditor.exe"
)

REM ======================================================
REM Run the executable
REM ======================================================
echo [3/3] Launching %EXE_NAME%...
set "EXE_PATH=%BUILD_DIR%\x64-%CONFIG%\%EXE_NAME%"

if exist "%EXE_PATH%" (
    start "" /D "%BUILD_DIR%\x64-%CONFIG%" "%EXE_NAME%"
) else (
    echo ERROR: Could not find executable at:
    echo   %EXE_PATH%
    goto :FAIL
)

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
echo Started : %start%
echo Finished: %end%
echo Total   : !minutes! min !seconds! sec
echo.

popd
endlocal
pause
