# ============================================================================
# CRASHOUTS ENGINE - MAIN TOP-LEVEL CMAKE
# ============================================================================
cmake_minimum_required(VERSION 3.25)
project(CrashoutEngine CXX)

# --- 1. Identify Build Target ---
set(TARGET_TO_ADD "Descent") # Default fallback
if(GAME_TARGET AND NOT GAME_TARGET STREQUAL "CrashoutEditor")
    set(TARGET_TO_ADD ${GAME_TARGET})
endif()

# --- 2. Diagnostic Header ---
message(STATUS "------------------------------------------------------------")
message(STATUS "  PROJECT       : ${PROJECT_NAME}")
message(STATUS "  GAME PROJECT  : ${TARGET_TO_ADD}")
message(STATUS "  BUILD MODE    : DUAL-TARGET (Standalone & Editor enabled)")
message(STATUS "------------------------------------------------------------")

# ======================= Project Configuration =========================
set(CMAKE_POLICY_VERSION_MINIMUM 3.5) 
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")

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

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

include(ImportDependencies)
importDependencies()

add_definitions(-DGLEW_STATIC)
add_definitions(-DUSE_CSD3151_AUTOMATION=0)
add_compile_definitions($<$<CONFIG:Debug>:DEBUG=1>)

include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/glm)
include_directories(${_FMOD_INC})
include_directories(${_FREETYPE_INC})
include_directories(${_MONO_INC})

if (MSVC)
    add_compile_options(/W4 /permissive-)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ========================= Build Sub-Projects ===========================

# 1. Build the engine libraries (Builds both standalone and editor versions)
add_subdirectory(projects/CrashoutEngine)

# 2. Add the Game Project (Builds both standalone and editor executables)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/projects/GameProjects/${TARGET_TO_ADD}/CMakeLists.txt")
    add_subdirectory(projects/GameProjects/${TARGET_TO_ADD})
else()
    message(FATAL_ERROR "[Crashouts] ERROR: Project folder not found: ${TARGET_TO_ADD}")
endif()