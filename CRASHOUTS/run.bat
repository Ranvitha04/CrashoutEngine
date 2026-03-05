@echo off
setlocal enabledelayedexpansion
pushd "%~dp0"

set "start=%time%"

:: ======================================================
:: Default settings
:: ======================================================
set "DEFAULT_TARGET=CrashoutEditor"
set "DEFAULT_CONFIG=Debug"
set "BUILD_DIR=build"
set "LAST_TARGET_FILE=%BUILD_DIR%\last_target.txt"
set "SLN_NAME=CrashoutEngine.sln"

:: ======================================================
:: Parse arguments
:: ======================================================
set "TARGET_INPUT=%~1"
set "CONFIG_INPUT=%~2"

if "%TARGET_INPUT%"=="" (
    set "TARGET=%DEFAULT_TARGET%"
) else if /i "%TARGET_INPUT%"=="Debug" (
    set "TARGET=%DEFAULT_TARGET%"
    set "CONFIG=Debug"
) else if /i "%TARGET_INPUT%"=="Release" (
    set "TARGET=%DEFAULT_TARGET%"
    set "CONFIG=Release"
) else (
    set "TARGET=%TARGET_INPUT%"
)

if not defined CONFIG (
    if /i "%CONFIG_INPUT%"=="Release" (set "CONFIG=Release") else (set "CONFIG=%DEFAULT_CONFIG%")
)

:: ======================================================
:: Smart Configuration Toggle
:: ======================================================
:: Check if we are switching between Editor and Standalone
if exist "%LAST_TARGET_FILE%" (
    set /p LAST_TARGET=<"%LAST_TARGET_FILE%"
) else (
    set "LAST_TARGET=None"
)

:: If target changed, we must clear CMakeCache so the Root flag updates
if not "%TARGET%"=="%LAST_TARGET%" (
    echo [Notice] Target changed from !LAST_TARGET! to %TARGET%. 
    echo          Clearing CMake cache to update Editor flags...
    if exist "%BUILD_DIR%\CMakeCache.txt" del /f /q "%BUILD_DIR%\CMakeCache.txt"
)

:: ======================================================
:: Project Validation
:: ======================================================
set "GAME_PROJ_PATH=projects\GameProjects\Descent"
if /i "%TARGET%"=="CrashoutEditor" (
    echo [Validated] Building Engine Editor
) else if exist "projects\GameProjects\%TARGET%\CMakeLists.txt" (
    echo [Validated] Building Game Project: %TARGET%
) else (
    echo ERROR: Target "%TARGET%" is not valid.
    goto :FAIL
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Save current target for next run
echo %TARGET%>"%LAST_TARGET_FILE%"

echo [1/3] Configuring project with CMake...
cmake -S . -B "%BUILD_DIR%" -A x64 -DGAME_TARGET=%TARGET%
if ERRORLEVEL 1 goto :FAIL

echo [2/3] Building %TARGET% (%CONFIG%)...
cmake --build "%BUILD_DIR%" --target %TARGET% --config %CONFIG% --parallel 8
if ERRORLEVEL 1 goto :FAIL

:: ======================================================
:: Launch Logic
:: ======================================================
:: If building Editor, the EXE name is always CrashoutEditor.exe
:: If building a game, it is GameName.exe
set "EXE_NAME=%TARGET%.exe"
set "EXE_PATH=%BUILD_DIR%\x64-%CONFIG%\%EXE_NAME%"

echo [3/3] Launching %EXE_NAME%...
if exist "%EXE_PATH%" (
    start "" /D "%~dp0%BUILD_DIR%\x64-%CONFIG%" "%EXE_NAME%"
) else (
    echo ERROR: Could not find executable at: %EXE_PATH%
    goto :FAIL
)

:: --- NEW: Open the Visual Studio Solution ---
set "SLN_PATH=%BUILD_DIR%\%SLN_NAME%"
if exist "%SLN_PATH%" (
    echo Opening Visual Studio Solution: %SLN_NAME%...
    start "" "%SLN_PATH%"
)

goto :TIMER

:TIMER
:: (Timer logic remains exactly as your original script)
set "end=%time%"
for /f "tokens=1-4 delims=:." %%a in ("%start%") do (set /a "startms=(1%%a-100)*3600000 + (1%%b-100)*60000 + (1%%c-100)*1000 + 1%%d-100")
for /f "tokens=1-4 delims=:." %%a in ("%end%") do (set /a "endms=(1%%a-100)*3600000 + (1%%b-100)*60000 + (1%%c-100)*1000 + 1%%d-100")
if !endms! lss !startms! set /a endms+=86400000
set /a elapsed=!endms!-!startms!
set /a minutes=!elapsed!/60000
set /a seconds=(!elapsed!%%60000)/1000
echo.
echo Started : %start% ^| Finished: %end% ^| Total: !minutes! min !seconds! sec
echo.

:FAIL
popd
endlocal
pause