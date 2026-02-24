@echo off
setlocal enabledelayedexpansion
pushd "%~dp0"

echo.
echo ======================================================
echo              CrashoutsEngine: Deep Clean
echo ======================================================

:: 1. Sync assets BACK to source projects so changes aren't lost
set "BUILD_DIR=build"
for /d %%D in (projects\GameProjects\*) do (
    set "GAME_NAME=%%~nxD"
    set "DEBUG_ASSETS=%BUILD_DIR%\bin\Debug\assets"
    set "SOURCE_ASSETS=%%D\assets"

    if exist "!DEBUG_ASSETS!" (
        echo Syncing modifications from build back to !GAME_NAME!...
        :: /XO excludes older files (preserves newer source edits)
        robocopy "!DEBUG_ASSETS!" "!SOURCE_ASSETS!" /E /XO /NJH /NJS /NDL /NC /NS
    )
)

:: 2. Wipe build artifacts
echo Cleaning build artifacts...
if exist %BUILD_DIR% (
    rmdir /s /q %BUILD_DIR%
    echo Removed %BUILD_DIR% directory.
)

if exist CMakeCache.txt del /f /q CMakeCache.txt
if exist CMakeFiles rmdir /s /q CMakeFiles

echo Cleanup complete.
popd
endlocal
pause