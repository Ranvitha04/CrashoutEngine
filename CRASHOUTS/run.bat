@echo off
setlocal
pushd "%~dp0"
set "BUILD_DIR=out\build"
set "ARCH=x64"
set "EXE_NAME=CrashoutEngine.exe"

REM --- Configuration Selection for Running (defaults to Debug) ---
REM Allows running as 'run.bat Release' or 'run.bat Debug'
set "CONFIG=%1"
if not defined CONFIG (
    set "CONFIG=Debug"
)
REM Normalize case for directory lookup
if /i "%CONFIG%"=="debug" set "CONFIG=Debug"
if /i "%CONFIG%"=="release" set "CONFIG=Release"

echo Configuration to Run: %CONFIG%

echo STEP 1: Ensure build dir exists...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo STEP 2: Remove old CMake cache and FetchContent sub-builds if present...
if exist "%BUILD_DIR%\CMakeCache.txt" (
    echo Removing old CMake cache and files to avoid generator/platform mismatch...
    rmdir /s /q "%BUILD_DIR%\CMakeFiles"
    del /f /q "%BUILD_DIR%\CMakeCache.txt"
)
if exist "%BUILD_DIR%\_deps" (
    echo Removing FetchContent _deps directory...
    rmdir /s /q "%BUILD_DIR%\_deps"
)
if exist "%BUILD_DIR%\_deps\glfw-build" (
    echo Removing glfw sub-build directory...
    rmdir /s /q "%BUILD_DIR%\_deps\glfw-build"
)

echo STEP 3: Configure (generate solution)...
cmake -S . -B "%BUILD_DIR%" -DCMAKE_POLICY_VERSION_MINIMUM=3.5
if ERRORLEVEL 1 goto FAIL_CONFIGURE

echo STEP 4: Build (Debug and Release)...
REM Build the Debug configuration
echo Building Debug...
cmake --build "%BUILD_DIR%" --config Debug
if ERRORLEVEL 1 goto FAIL_BUILD

REM Build the Release configuration
echo Building Release...
cmake --build "%BUILD_DIR%" --config Release
if ERRORLEVEL 1 goto FAIL_BUILD

echo STEP 5: Locate your executable deterministically in %CONFIG%...

set "FOUND_EXE="
REM Note: %CONFIG% is used here to target the specific executable directory
set "CAND1=%BUILD_DIR%\%CONFIG%\%EXE_NAME%"
set "CAND2=%BUILD_DIR%\%EXE_NAME%"
set "CAND3=%BUILD_DIR%\bin\%CONFIG%\%EXE_NAME%"

if exist "%CAND1%" set "FOUND_EXE=%CAND1%"
if not defined FOUND_EXE if exist "%CAND2%" set "FOUND_EXE=%CAND2%"
if not defined FOUND_EXE if exist "%CAND3%" set "FOUND_EXE=%CAND3%"

if not defined FOUND_EXE (
    for /R "%BUILD_DIR%" %%F in ("%EXE_NAME%") do (
        set "FOUND_EXE=%%~fF"
        goto :FOUND_EXE_LABEL
    )
)

if not defined FOUND_EXE (
    for /R "%BUILD_DIR%" %%F in (*.exe) do (
        echo %%~nF | findstr /i "CompilerIdC" >nul
        if errorlevel 1 (
            set "FOUND_EXE=%%~fF"
            goto :FOUND_EXE_LABEL
        )
    )
)

:FOUND_EXE_LABEL
if defined FOUND_EXE (
    echo Found executable: %FOUND_EXE%
    echo Launching game...

    REM Find the directory of the found executable (%%~dpF gets path)
    for %%F in ("%FOUND_EXE%") do set "EXE_DIR=%%~dpF"

    REM Force working directory to the executable's directory (where DLLs/Assets should be)
    pushd "%EXE_DIR%"
    echo Current working directory: %CD%

    REM Execute the program using the full path
    "%FOUND_EXE%"

    popd
) else (
    echo ERROR: No executable found for configuration %CONFIG%. Inspect build output.
    goto END
)

echo SUCCESS: launched executable.
goto END

:FAIL_BUILD
echo CMake Build FAILED for one or more configurations. See messages above.
goto END

:FAIL_CONFIGURE
echo CMake Configure FAILED. See messages above.
:END
popd
endlocal
pause


so how do i edit it to that