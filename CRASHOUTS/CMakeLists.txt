# ======================= Project Configuration =========================
cmake_minimum_required(VERSION 2.8...3.13)

# Set the CMake policy version (this enables all policies up to CMake 3.5)
cmake_policy(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(CRASHOUTS)

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
    PRIVATE refl-cpp::refl-cpp
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
    # Clear warning level flags and enforce /W4 /WX
    string (REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /permissive- /WX")
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
        # Set C++ standard
        set_property(TARGET ${child} PROPERTY CXX_STANDARD 20)

        # Enable WIN32
        set_property(TARGET ${child} PROPERTY WIN32 TRUE)
        #[[
        # Warnings based on compiler
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${child} PRIVATE -Wall ${DisableWarnings})
        elseif (MSVC)
            target_compile_options(${child} PRIVATE /W3 /WX-)
        endif()
        ]]
        # Include path for each project
        target_include_directories(${child} PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/projects/${child}/include
        )
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
