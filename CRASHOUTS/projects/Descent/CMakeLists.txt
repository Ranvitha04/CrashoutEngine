#projects/Descent/CMakeLists.txt

# Project name for the DLL file.
project(Descent)

# Set the project directory variable for convenience
set(project_dir ${CMAKE_CURRENT_LIST_DIR})

# --- All file (GLOB_RECURSE...) commands go here ---
message(STATUS "Configuring project: ${PROJECT_NAME}")

# --- 1. GLOB your specific categories ---

# --- 2. GLOB ALL source and include files separately ---
file(GLOB_RECURSE all_src_files     "${project_dir}/src/*.cpp")
file(GLOB_RECURSE all_include_files "${project_dir}/include/*.h" "${project_dir}/include/*.hpp")

# --- 3. Create lists of all categorized files ---
#set(categorized_src_files)

#set(categorized_include_files)

# --- 4. Subtract the categorized lists to find the "other" files ---
#list(REMOVE_ITEM all_src_files     ${categorized_src_files})
#list(REMOVE_ITEM all_include_files ${categorized_include_files})
set(other_src_files     ${all_src_files})
set(other_include_files ${all_include_files})

# --- 5. Create the executable (EXE) with ALL files ---
add_executable(${PROJECT_NAME} WIN32
    ${other_src_files}
    ${other_include_files}
)

# Tell CMake to use pch.h as the precompiled header
target_precompile_headers(${PROJECT_NAME}
    PRIVATE
        "${project_dir}/include/pch.h"
)

# --- 6. Assign ALL file groups to filters ✨ ---
source_group("src"     FILES ${other_src_files})
source_group("include" FILES ${other_include_files})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    CrashoutEngine
    #CrashoutsAudio
)
#link_fmod_to(${PROJECT_NAME}) 
link_mono_to(${PROJECT_NAME})

# Apply common include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${project_dir}/include
)
        
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

# Copy assets (Simplified path variables)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    "${project_dir}/assets"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

# Ensure C# scripts (ManagedScripts) are built before Descent
if (TARGET ManagedScripts)
    add_dependencies(Descent ManagedScripts)
endif()


