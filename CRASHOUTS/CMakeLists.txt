# MAIN TOP-LEVEL CMAKE FILE

# ======================= Project Configuration =========================
cmake_minimum_required(VERSION 3.5)

# Set the CMake policy version (this enables all policies up to CMake 3.5)
cmake_policy(VERSION 3.24)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Normal configurations + a special one for C++/CLI debug builds
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;CLR-Debug" CACHE STRING "" FORCE)

project(CrashoutEngine CXX)

# Set a single, clean output directory for all binaries.
# This puts all .exe, .dll, and .lib files in the same place.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the directory where custom CMake modules are stored
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Disable GLFW examples, tests and docs to avoid extra executables
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

# Include project-wide utility and dependency modules
include(ImportDependencies)
importDependencies()

# Global compile flags and macros
add_definitions(-DGLEW_STATIC)
add_definitions(-DUSE_CSD3151_AUTOMATION=0) # Used for instructor's automation

add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG=1>
)

# Include path for GLM (optional if not using FetchContent)
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/glm)
include_directories(${_FMOD_INC})

# ======================= Global Warning Level =========================
if (MSVC)
    # Use target_compile_options to ensure flags are applied correctly and universally
    add_compile_options(
        /W4          # Level 4 warnings (Good)
        /permissive- # Strict conformance
        /WX          # Warnings as Errors
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ========================= Build Sub-Projects ===========================
# This section explicitly tells CMake to build the engine first, then the game.
#
add_subdirectory(projects/CrashoutEngine)
add_subdirectory(projects/ScriptAPI)
add_subdirectory(projects/Descent)

# ========================= ManagedScripts (.NET 6) ======================
include(ExternalProject)

ExternalProject_Add(ManagedScripts
SOURCE_DIR "${CMAKE_SOURCE_DIR}/projects/ManagedScripts"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND dotnet build ManagedScripts.csproj -c $<CONFIG> -p:Platform=x64
    INSTALL_COMMAND ""
)

# Make sure it builds after ScriptAPI
add_dependencies(ManagedScripts ScriptAPI)
