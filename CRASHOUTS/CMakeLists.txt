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

# --- FMOD: imported libs live in CRASHOUTS/third_party/fmod ---
set(FMOD_DIR "${CMAKE_SOURCE_DIR}/third_party/fmod")
set(FMOD_INCLUDE_DIR "${FMOD_DIR}/include")   

add_library(fmod SHARED IMPORTED GLOBAL)
set_target_properties(fmod PROPERTIES               
  IMPORTED_CONFIGURATIONS "Debug;Release"
  IMPORTED_IMPLIB_RELEASE   "${FMOD_DIR}/lib/x64/fmod_vc.lib"
  IMPORTED_LOCATION_RELEASE "${FMOD_DIR}/bin/x64/fmod.dll"
  IMPORTED_IMPLIB_DEBUG     "${FMOD_DIR}/lib/x64/fmodL_vc.lib"
  IMPORTED_LOCATION_DEBUG   "${FMOD_DIR}/bin/x64/fmodL.dll"
)
# studio
add_library(fmodstudio SHARED IMPORTED GLOBAL)
set_target_properties(fmodstudio PROPERTIES       
  IMPORTED_CONFIGURATIONS "Debug;Release"
  IMPORTED_IMPLIB_RELEASE   "${FMOD_DIR}/lib/x64/fmodstudio_vc.lib"
  IMPORTED_LOCATION_RELEASE "${FMOD_DIR}/bin/x64/fmodstudio.dll"
  IMPORTED_IMPLIB_DEBUG     "${FMOD_DIR}/lib/x64/fmodstudioL_vc.lib"
  IMPORTED_LOCATION_DEBUG   "${FMOD_DIR}/bin/x64/fmodstudioL.dll"
)
set(FMOD_INCLUDE_DIR "${FMOD_DIR}/include")

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
)

# Include path for GLM (optional if not using FetchContent)
include_directories(${CMAKE_CURRENT_LIST_DIR}/lib/glm)

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
        # Link libraries
        target_link_libraries(${child}
            ${ALL_LIBS}
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
         # ---------- FMOD for CrashoutEngine only ----------
        if (${child} STREQUAL "CrashoutEngine")
          target_include_directories(${child} PRIVATE "${FMOD_INCLUDE_DIR}")   # <-- CHANGED: uses FMOD_INCLUDE_DIR
          target_link_libraries(${child} PRIVATE fmodstudio fmod)

          # Copy the correct FMOD DLLs next to the .exe (handles Debug/Release)  <-- NEW
          add_custom_command(TARGET ${child} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "$<IF:$<CONFIG:Debug>,${FMOD_DIR}/bin/x64/fmodL.dll,${FMOD_DIR}/bin/x64/fmod.dll>"
              "$<TARGET_FILE_DIR:${child}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "$<IF:$<CONFIG:Debug>,${FMOD_DIR}/bin/x64/fmodstudioL.dll,${FMOD_DIR}/bin/x64/fmodstudio.dll>"
              "$<TARGET_FILE_DIR:${child}>"
          )

          # Copy your .bank files next to the .exe so relative paths work       <-- NEW
          add_custom_command(TARGET ${child} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
              "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/assets/audio/banks"
              "$<TARGET_FILE_DIR:${child}>/assets/audio/banks"
          )
        endif()     # closes STREQUAL "CrashoutEngine"    
    endif()
endforeach()

# Doxygen: rebuild only when files change
find_package(Doxygen QUIET)

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