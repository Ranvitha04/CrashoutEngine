# MAIN TOP-LEVEL CMAKE FILE

# ======================= Project Configuration =========================
cmake_minimum_required(VERSION 3.25)
project(CrashoutEngine CXX)

# Allow older dependencies (e.g. GLEW) to configure under modern CMake
set(CMAKE_POLICY_VERSION_MINIMUM 3.5) 

# Force a consistent root binary directory (so subdirs don't nest)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/x64-$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/x64-$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/x64-$<CONFIG>")

foreach(OUTPUTCONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_SOURCE_DIR}/build/x64-${OUTPUTCONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_SOURCE_DIR}/build/x64-${OUTPUTCONFIG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_SOURCE_DIR}/build/x64-${OUTPUTCONFIG}")
endforeach()

# ======================= Compiler Settings =========================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

# ======================= Module and Dependency Setup ====================
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Disable GLFW extras to avoid unnecessary builds
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

# Include project-wide dependency importer
include(ImportDependencies)
importDependencies()

# ======================= Compile Definitions ============================
add_definitions(-DGLEW_STATIC)
add_definitions(-DUSE_CSD3151_AUTOMATION=0) # For instructor automation

add_compile_definitions(
    $<$<CONFIG:Debug>:DEBUG=1>
    USE_IMGUI_EDITOR=1
)

# Include path for GLM (optional if not using FetchContent)
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/glm)
include_directories(${_FMOD_INC})
include_directories(${_MONO_INC})

# ======================= Global Warning Level ===========================
if (MSVC)
    add_compile_options(
        /W4          # Level 4 warnings
        /permissive- # Strict standard conformance
        # /WX         # Uncomment to treat warnings as errors
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ========================= Build Sub-Projects ===========================
# Build the engine first, then the game.
add_subdirectory(projects/CrashoutEngine)
add_subdirectory(projects/Descent)
