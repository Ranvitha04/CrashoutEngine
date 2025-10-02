@echo off
REM --- Configuration Options ---
set "BUILD_DIR=build"
set "PROJECT_NAME=CrashoutEngine"
set "BUILD_TYPE=Debug"

REM --- Set the current directory to the script's directory ---
PUSHD "%~dp0"

REM --- 1. Configure the project ---
IF NOT EXIST "%BUILD_DIR%" (
    echo [Step 1/3] Creating and configuring build directory: %BUILD_DIR%
    cmake -S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE="%BUILD_TYPE%"
    IF ERRORLEVEL 1 (
        echo CMake Configure FAILED!
        GOTO :END
    )
) ELSE (
    echo [Step 1/3] Build directory already exists. Skipping configure.
    REM You might want to re-run cmake config here if CMakeLists.txt changes a lot
)

REM --- 2. Build the project ---
echo.
echo [Step 2/3] Building the project...
REM Using --config is necessary for multi-config generators like Visual Studio
cmake --build "%BUILD_DIR%" --config "%BUILD_TYPE%"
IF ERRORLEVEL 1 (
    echo CMake Build FAILED!
    GOTO :END
)

REM --- 3. Execute the program ---
echo.
echo [Step 3/3] Running the executable: %PROJECT_NAME%.exe
REM The executable is typically located in the build sub-directory corresponding to the configuration type
"%BUILD_DIR%\%BUILD_TYPE%\%PROJECT_NAME%.exe"
IF ERRORLEVEL 1 (
    echo Execution FAILED!
)

:END
echo.
pause
POPD