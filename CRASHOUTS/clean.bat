@echo off
setlocal
pushd "%~dp0"

echo Saving modified assets...

set "SOURCE_ASSETS=projects\Descent\assets"
set "BUILD_DIR=build"
set "DEBUG_ASSETS=%BUILD_DIR%\Debug\assets"
set "RELEASE_ASSETS=%BUILD_DIR%\Release\assets"

REM Check for Debug assets and copy only newer files back to source
if exist "%DEBUG_ASSETS%" (
    echo Syncing from Debug...
    robocopy "%DEBUG_ASSETS%" "%SOURCE_ASSETS%" /E /XO
)

REM Check for Release assets and copy only newer files back to source
if exist "%RELEASE_ASSETS%" (
    echo Syncing from Release...
    robocopy "%RELEASE_ASSETS%" "%SOURCE_ASSETS%" /E /XO
)

echo.
echo Cleaning build artifacts...

REM Delete the 'build' directory completely
if exist build (
  rmdir /s /q build
  echo Removed build directory.
) else (
  echo No build directory found.
)

REM Optionally remove cache/config files if any left
if exist CMakeCache.txt del /f /q CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles

echo Cleanup complete.
popd
endlocal
pause