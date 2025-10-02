@echo off
setlocal
pushd "%~dp0"

echo Cleaning build artifacts...

REM Delete the build directory completely
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
