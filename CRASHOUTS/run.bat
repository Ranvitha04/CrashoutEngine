@echo off
setlocal
pushd "%~dp0"
set "BUILD_DIR=build"
set "CONFIG=Debug"
set "ARCH=x64"
set "EXE_NAME=THECARBS_code.exe"

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
if ERRORLEVEL 1 goto FAIL

echo STEP 4: Build (%CONFIG%)...
cmake --build "%BUILD_DIR%" --config %CONFIG%
if ERRORLEVEL 1 goto FAIL

echo STEP 5: Locate your executable deterministically...

set "FOUND_EXE="
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

    REM Execute the program using the full path, which now includes the correct directory
    REM Or, since we CD'd into the directory, we can use the short name:
    "%FOUND_EXE%"

    popd
) else (
    echo ERROR: No executable found. Inspect build output.
    goto END
)

echo SUCCESS: launched executable.
goto END

:FAIL
echo BUILD FAILED. See messages above.
:END
popd
endlocal
pause