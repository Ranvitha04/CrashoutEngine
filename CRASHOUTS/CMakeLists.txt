# ======================= Project Configuration =========================
cmake_minimum_required(VERSION 2.8...3.13)

# Set the CMake policy version (this enables all policies up to CMake 3.5)
cmake_policy(VERSION 3.5)

project(CRASHOUTS)

# Set the directory where custom CMake modules are stored
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

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

)

add_compile_definitions(
    $<$<CONFIG:Debug>:-DDEBUG=1>
)

# Include path for GLM (optional if not using FetchContent)
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/glm)
# --- FMOD (vendored in repo) ---
set(FMOD_INC_CORE   "${CMAKE_SOURCE_DIR}/third_party/fmod/include")
set(FMOD_INC_STUDIO "${CMAKE_SOURCE_DIR}/third_party/fmod/include")
set(FMOD_LIB_DIR_CORE   "${CMAKE_SOURCE_DIR}/third_party/fmod/lib/x64")
set(FMOD_LIB_DIR_STUDIO "${CMAKE_SOURCE_DIR}/third_party/fmod/lib/x64")
set(FMOD_DLL_CORE       "${CMAKE_SOURCE_DIR}/third_party/fmod/bin/x64/fmod.dll")
set(FMOD_DLL_STUDIO     "${CMAKE_SOURCE_DIR}/third_party/fmod/bin/x64/fmodstudio.dll")

# Fail fast if something is missing
if(NOT EXISTS "${FMOD_INC_CORE}/fmod.hpp")
  message(FATAL_ERROR "Missing header: ${FMOD_INC_CORE}/fmod.hpp")
endif()
if(NOT EXISTS "${FMOD_INC_STUDIO}/fmod_studio.hpp")
  message(FATAL_ERROR "Missing header: ${FMOD_INC_STUDIO}/fmod_studio.hpp")
endif()
if(NOT EXISTS "${FMOD_LIB_DIR_CORE}/fmod_vc.lib")
  message(FATAL_ERROR "Missing lib: ${FMOD_LIB_DIR_CORE}/fmod_vc.lib")
endif()
if(NOT EXISTS "${FMOD_LIB_DIR_STUDIO}/fmodstudio_vc.lib")
  message(FATAL_ERROR "Missing lib: ${FMOD_LIB_DIR_STUDIO}/fmodstudio_vc.lib")
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
        add_executable(${child}
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
        )
        # --- FMOD: add include paths for this target ---
        target_include_directories(${child} PRIVATE
            "${FMOD_INC_CORE}"
            "${FMOD_INC_STUDIO}"
        )

        # --- FMOD: link directories + libraries ---
        target_link_directories(${child} PRIVATE
          "${FMOD_LIB_DIR_CORE}"
          "${FMOD_LIB_DIR_STUDIO}"
        )
        # MSVC import lib names (64-bit). For logging builds use fmodL_vc / fmodstudioL_vc.
        target_link_libraries(${child} PRIVATE fmod_vc fmodstudio_vc)

        # --- FMOD: copy runtime DLLs next to the exe so it runs without PATH edits ---
        add_custom_command(TARGET ${child} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FMOD_DLL_CORE}"   "$<TARGET_FILE_DIR:${child}>/fmod.dll"
        )
        add_custom_command(TARGET ${child} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FMOD_DLL_STUDIO}" "$<TARGET_FILE_DIR:${child}>/fmodstudio.dll"
        )

        # Set C++ standard
        set_property(TARGET ${child} PROPERTY CXX_STANDARD 20)

        # Warnings based on compiler
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${child} PRIVATE -Wall ${DisableWarnings})
        elseif (MSVC)
            target_compile_options(${child} PRIVATE /W3 /WX-)
        endif()

        # Include path for each project
        target_include_directories(${child} PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/projects/${child}/include
        )
    endif()
endforeach()

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