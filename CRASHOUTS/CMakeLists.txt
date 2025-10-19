if (MSVC)
    message(STATUS "Disabling /WX (treat warnings as errors) globally...")

    # Ensure /WX- always wins for all targets and dependencies
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/WX->)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:/WX->)

    # Patch compiler flags globally so even nested FetchContent targets obey
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX-")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX-")

    # This hook runs after all subprojects are created
    set(CMAKE_POLICY_DEFAULT_CMP0092 NEW)
endif()

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

# Disable reflect internal builds
set(REFLECT_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(REFLECT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(REFLECT_BUILD_TESTS OFF CACHE BOOL "" FORCE)

set(IMGUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(IMGUI_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(IMGUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)

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

# ======================= CORE / SCRIPTING SETUP =========================

# ---------- Core Engine (DLL build) ----------
file(GLOB_RECURSE CORE_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/projects/CrashoutEngine/Core/*.cpp"
)
file(GLOB_RECURSE CORE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/projects/CrashoutEngine/Core/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/projects/CrashoutEngine/Core/*.hpp"
)

# Build as a shared library (DLL)
add_library(CrashoutCore SHARED ${CORE_SRC} ${CORE_HEADERS})
target_compile_definitions(CrashoutCore PRIVATE DLL_API_EXPORT)

# ---------- FMOD Setup ----------
set(_FMOD_INC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmod/include")
set(_FMOD_LIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmod/lib/x64")

# Ensure FMOD libs are visible to the linker (Debug + Release)
link_directories("${_FMOD_LIB}")

message(STATUS "FMOD include path: ${_FMOD_INC}")
message(STATUS "FMOD lib path: ${_FMOD_LIB}")
file(GLOB FMOD_LIBS "${_FMOD_LIB}/*.lib")
message(STATUS "FMOD libs found: ${FMOD_LIBS}")


target_include_directories(CrashoutCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/projects/CrashoutEngine/Core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/projects/CrashoutEngine/Core
    ${CMAKE_CURRENT_LIST_DIR}/lib/glm
    ${_FMOD_INC}
)

target_link_directories(CrashoutCore PUBLIC
    ${_FMOD_LIB}
)


target_link_libraries(CrashoutCore PUBLIC
    glfw
    libglew_static
    imgui
    glm::glm
    stb
    freetype
    reflect
    rapidjson
    $<$<CONFIG:Debug>:fmodL_vc.lib>
    $<$<CONFIG:Debug>:fmodstudioL_vc.lib>
    $<$<CONFIG:Release>:fmod_vc.lib>
    $<$<CONFIG:Release>:fmodstudio_vc.lib>
)

if (MSVC)
    # Disable "warnings as errors" globally and silence override spam
    add_compile_options(/WX-)
    string(APPEND CMAKE_CXX_FLAGS " /WX-")
    string(APPEND CMAKE_C_FLAGS " /WX-")
endif()



# ---------- ScriptAPI (C++/CLI Layer) ----------
if (MSVC)
    file(GLOB_RECURSE SCRIPTAPI_SRC "${CMAKE_CURRENT_SOURCE_DIR}/projects/CrashoutEngine/ScriptAPI/*.cpp")
    add_library(ScriptAPI SHARED ${SCRIPTAPI_SRC})
    set_target_properties(ScriptAPI PROPERTIES COMMON_LANGUAGE_RUNTIME "" CXX_STANDARD 17)
    target_link_libraries(ScriptAPI PRIVATE CrashoutCore)
    target_include_directories(ScriptAPI PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/projects/CrashoutEngine/Core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/projects/CrashoutEngine/Core
        ${_FMOD_INC}
    )
endif()

# ---------- ManagedScripts (C# Project) ----------
set(CSHARP_PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/managedscripts/ManagedScripts.csproj")
if (EXISTS ${CSHARP_PROJECT_PATH})
    add_custom_target(ManagedScripts ALL
        COMMAND dotnet build ${CSHARP_PROJECT_PATH} -c $<CONFIG> -o "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/managedscripts
        COMMENT "Building Managed C# Scripts...")
endif()

# ---------- Main Executable ----------
file(GLOB_RECURSE MAIN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
list(FILTER MAIN_SRC EXCLUDE REGEX ".*/_deps/.*")

add_executable(CrashoutEngine ${MAIN_SRC})
if (WIN32)
    # Use Windows subsystem so it looks for WinMain instead of main
    target_link_options(CrashoutEngine PRIVATE "/SUBSYSTEM:WINDOWS")
endif()
# Copy FMOD DLLs to the output directory after build
add_custom_command(TARGET CrashoutEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/third_party/fmod/bin/x64/fmodL.dll"
        "${CMAKE_SOURCE_DIR}/third_party/fmod/bin/x64/fmodstudioL.dll"
        "$<TARGET_FILE_DIR:CrashoutEngine>"
)

# Define macro so headers know to import from the DLL
target_compile_definitions(CrashoutEngine PRIVATE DLL_API_IMPORT)

# Ensure the EXE builds *after* the DLL
add_dependencies(CrashoutEngine CrashoutCore)

# Link directly to the target (not the .lib file path)
target_link_libraries(CrashoutEngine PRIVATE CrashoutCore)

# Include directories for engine headers and FMOD
target_include_directories(CrashoutEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/projects/CrashoutEngine/Core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/projects/CrashoutEngine/Core
    ${_FMOD_INC}
)

# ---------- Output Directories ----------
foreach(target CrashoutCore ScriptAPI CrashoutEngine)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
        )
    endif()
endforeach()


# ======================= AUTO PROJECT DISCOVERY =========================
# Get all subdirectories in the projects folder
file(GLOB children RELATIVE "${CMAKE_CURRENT_LIST_DIR}/projects/CrashoutEngine/" "${CMAKE_CURRENT_LIST_DIR}/projects/CrashoutEngine/*")

foreach(child ${children})
    # Skip engine-level folders that are not part of game projects
    if(child STREQUAL "Core" OR child STREQUAL "ScriptAPI" OR child STREQUAL "managedscripts")
        message(STATUS "Skipping engine subproject: ${child}")
        continue()
    endif()

    if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/projects/CrashoutEngine/${child}")
        message(STATUS "Configuring project: ${child}")

        set(project_dir "${CMAKE_CURRENT_LIST_DIR}/projects/CrashoutEngine/${child}")
        if(EXISTS "${project_dir}/Core")
            set(project_dir "${project_dir}/Core")
            message(STATUS "Detected nested Core folder in ${child}, using ${project_dir}")
        endif()
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
        set_target_properties(${child} PROPERTIES OUTPUT_NAME "CrashoutEngine")
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

if (MSVC)
    get_property(all_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
    foreach(tgt ${all_targets})
        get_target_property(flags ${tgt} COMPILE_OPTIONS)
        if (flags)
            list(REMOVE_ITEM flags "/WX")
            list(REMOVE_ITEM flags "/WX-")
            list(APPEND flags "/WX-")  # force it off once
            set_target_properties(${tgt} PROPERTIES COMPILE_OPTIONS "${flags}")
        endif()
    endforeach()
endif()
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