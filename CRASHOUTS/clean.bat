@echo off
setlocal
pushd "%~dp0"

REM echo Saving modified assets...

REM set "SOURCE_ASSETS=projects\Descent\assets"
REM set "BUILD_DIR=build"
REM set "DEBUG_ASSETS=%BUILD_DIR%\x64-Debug\assets"
REM set "RELEASE_ASSETS=%BUILD_DIR%\x64-Release\assets"

REM Check for Debug assets and copy only newer files back to source
REM if exist "%DEBUG_ASSETS%" (
REM    echo Syncing from Debug...
REM    robocopy "%DEBUG_ASSETS%" "%SOURCE_ASSETS%" /E /XO
REM )

REM Check for Release assets and copy only newer files back to source
REM if exist "%RELEASE_ASSETS%" (
REM    echo Syncing from Release...
REM    robocopy "%RELEASE_ASSETS%" "%SOURCE_ASSETS%" /E /XO
REM )

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