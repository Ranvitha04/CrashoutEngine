# ======================= Project Configuration =========================
cmake_minimum_required(VERSION 3.5)

# Set the CMake policy version (this enables all policies up to CMake 3.5)
cmake_policy(VERSION 3.24)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(CRASHOUTS CXX)

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

# List of external libraries
set(ALL_LIBS
    PRIVATE glfw
    PRIVATE libglew_static
    PRIVATE imgui
    PRIVATE glm::glm
    PRIVATE stb
    PRIVATE freetype
    PRIVATE reflect
    PRIVATE rapidjson
)

add_compile_definitions(
    $<$<CONFIG:Debug>:-DDEBUG=1>
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

# ======================= AUTO PROJECT DISCOVERY =========================
# Get all subdirectories in the projects folder
file(GLOB children RELATIVE "${CMAKE_CURRENT_LIST_DIR}/projects" "${CMAKE_CURRENT_LIST_DIR}/projects/*")

foreach(child ${children})
    if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/projects/${child}")
        message(STATUS "Configuring project: ${child}")

        # Collect sources
        file(GLOB_RECURSE ${child}_source_files
            ${CMAKE_CURRENT_LIST_DIR}/projects/${child}/src/*.cpp
            ${CMAKE_CURRENT_LIST_DIR}/projects/${child}/include/*.h
            ${CMAKE_CURRENT_LIST_DIR}/projects/${child}/include/*.hpp
            ${CMAKE_CURRENT_LIST_DIR}/projects/${child}/state/*.h
            ${CMAKE_CURRENT_LIST_DIR}/projects/${child}/state/*.hpp
        )

        # Create executable
        add_executable(${child} WIN32
            ${${child}_source_files}
        )
        add_custom_command(TARGET ${child} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_LIST_DIR}/projects/${child}/assets
            $<TARGET_FILE_DIR:${child}>/assets
)

        # Link libraries
        target_link_libraries(${child}
            ${ALL_LIBS}
            CrashoutsAudio
        )
        link_fmod_to(${child}) 

        # Apply common include directories
        target_include_directories(${child} PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/projects/${child}/include
            ${CMAKE_CURRENT_LIST_DIR}/lib/glm           # Added GLM include
            ${_FMOD_INC}                                # Added FMOD include
        )
        
        # Copy assets (Simplified path variables)
        add_custom_command(TARGET ${child} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_LIST_DIR}/projects/${child}/assets
            $<TARGET_FILE_DIR:${child}>/assets
        )
        
        # WIN32 property only needs to be set once
        set_property(TARGET ${child} PROPERTY WIN32 TRUE)
    endif()
endforeach()
#[[
# Doxygen: rebuild only when files change
find_package(Doxygen REQUIRED)

set(DOXYFILE_IN ${CMAKE_SOURCE_DIR}/Doxyfile)
set(DOC_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/docs)

# Everything that should trigger a doc rebuild
file(GLOB_RECURSE DOXY_INPUTS CONFIGURE_DEPENDS
     ${CMAKE_SOURCE_DIR}/include/*.h
     ${CMAKE_SOURCE_DIR}/include/*.hpp
     ${CMAKE_SOURCE_DIR}/src/*.h
     ${CMAKE_SOURCE_DIR}/src/*.hpp
     ${CMAKE_SOURCE_DIR}/src/*.c
     ${CMAKE_SOURCE_DIR}/src/*.cc
     ${CMAKE_SOURCE_DIR}/src/*.cpp
     ${CMAKE_SOURCE_DIR}/*.md
     ${CMAKE_SOURCE_DIR}/docs/*.md)

# Produce a stamp file when doxygen finishes, depend on all inputs
add_custom_command(
    OUTPUT ${DOC_OUTPUT_DIR}/.doxygen.stamp
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DOC_OUTPUT_DIR}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_IN}
    COMMAND ${CMAKE_COMMAND} -E touch ${DOC_OUTPUT_DIR}/.doxygen.stamp
    BYPRODUCTS ${DOC_OUTPUT_DIR}/html/index.html
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running Doxygen on every build"
    VERBATIM
)

add_custom_target(doxygen_docs ALL
    DEPENDS ${DOC_OUTPUT_DIR}/.doxygen.stamp
)
]]