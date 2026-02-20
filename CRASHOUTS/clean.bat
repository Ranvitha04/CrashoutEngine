@echo off
setlocal
pushd "%~dp0"

REM echo Saving modified assets for all game projects...
REM set "BUILD_DIR=build"

REM Loop through every folder inside GameProjects
REM for /d %%D in (projects\GameProjects\*) do (
REM     set "GAME_NAME=%%~nxD"
REM     echo Syncing assets for !GAME_NAME!...
REM     set "SOURCE_ASSETS=%%D\assets"
REM     set "DEBUG_ASSETS=%BUILD_DIR%\x64-Debug\assets"
REM     set "RELEASE_ASSETS=%BUILD_DIR%\x64-Release\assets"
REM
REM     if exist "!DEBUG_ASSETS!" (
REM         robocopy "!DEBUG_ASSETS!" "!SOURCE_ASSETS!" /E /XO
REM     )
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