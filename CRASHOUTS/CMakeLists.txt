# ======================= Project Configuration =========================
cmake_minimum_required(VERSION 3.5)

# Set the CMake policy version (this enables all policies up to CMake 3.5)
cmake_policy(VERSION 3.24)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(CrashoutEngine CXX)

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

        set(project_dir "${CMAKE_CURRENT_LIST_DIR}/projects/${child}")

        # --- 1. GLOB your specific categories ---
        file(GLOB_RECURSE manager_src_files     "${project_dir}/src/*manager.*")
        file(GLOB_RECURSE manager_include_files "${project_dir}/include/*manager.*")

        file(GLOB_RECURSE component_src_files     "${project_dir}/src/*component.*")
        file(GLOB_RECURSE component_include_files "${project_dir}/include/*component.*")

        file(GLOB_RECURSE scene_src_files     "${project_dir}/src/*scene.*")
        file(GLOB_RECURSE scene_include_files "${project_dir}/include/*scene.*")

        file(GLOB_RECURSE resource_src_files     "${project_dir}/src/*asset.*")
        file(GLOB_RECURSE resource_include_files "${project_dir}/include/*asset.*")

        file(GLOB_RECURSE debug_src_files     "${project_dir}/src/debug*.*")
        file(GLOB_RECURSE debug_include_files "${project_dir}/include/debug*.*")

        file(GLOB_RECURSE message_include_files 
        "${project_dir}/include/messages.h"
        "${project_dir}/include/observable.h"
        "${project_dir}/include/observer.h")

        file(GLOB_RECURSE math_include_files 
        "${project_dir}/include/CEMath.h"
        "${project_dir}/include/CEMtx33.h"
        "${project_dir}/include/CETypes.h"
        "${project_dir}/include/CEVec2.h"
        "${project_dir}/include/CEVec3.h")

        # --- 2. GLOB ALL source and include files separately ---
        file(GLOB_RECURSE all_src_files     "${project_dir}/src/*.cpp")
        file(GLOB_RECURSE all_include_files "${project_dir}/include/*.h" "${project_dir}/include/*.hpp")

        # --- 3. Create lists of all categorized files ---
        set(categorized_src_files ${manager_src_files} ${component_src_files} ${scene_src_files} ${resource_src_files}
        ${debug_src_files})

        set(categorized_include_files ${manager_include_files} ${component_include_files} ${scene_include_files} 
        ${resource_include_files} ${debug_include_files} ${message_include_files} ${math_include_files})

        # --- 4. Subtract the categorized lists to find the "other" files ---
        list(REMOVE_ITEM all_src_files     ${categorized_src_files})
        list(REMOVE_ITEM all_include_files ${categorized_include_files})
        set(other_src_files     ${all_src_files})
        set(other_include_files ${all_include_files})

        # --- 5. Create the executable with ALL files ---
        add_executable(${child} WIN32
            ${categorized_src_files}
            ${categorized_include_files}
            ${other_src_files}
            ${other_include_files}
        )

        # --- 6. Assign ALL file groups to filters ✨ ---
        source_group("src\\Managers"   FILES ${manager_src_files})
        source_group("include\\Managers"   FILES ${manager_include_files})

        source_group("src\\Components" FILES ${component_src_files})
        source_group("include\\Components" FILES ${component_include_files})

        source_group("src\\Scenes"     FILES ${scene_src_files})
        source_group("include\\Scenes"     FILES ${scene_include_files})

        source_group("src\\Resources"     FILES ${resource_src_files})
        source_group("include\\Resources"     FILES ${resource_include_files})

        source_group("src\\Debug"     FILES ${debug_src_files})
        source_group("include\\Debug"     FILES ${debug_include_files})

        source_group("include\\Messaging"     FILES ${message_include_files})
        
        source_group("include\\Math"     FILES ${math_include_files})

        source_group("src"     FILES ${other_src_files})
        source_group("include" FILES ${other_include_files})

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
        
        set_property(TARGET ${child} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${child}>")

        # Copy assets (Simplified path variables)
        add_custom_command(TARGET ${child} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${project_dir}/assets"
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