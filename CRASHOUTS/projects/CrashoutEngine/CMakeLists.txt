#projects/CrashoutEngine/CMakeLists.txt

# Project name for the DLL file.
project(CrashoutEngine)

# Set the project directory variable for convenience
set(project_dir ${CMAKE_CURRENT_LIST_DIR})

# --- All file (GLOB_RECURSE...) commands go here ---
message(STATUS "Configuring project: ${PROJECT_NAME}")

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

# --- 5. Create the library (lib) with ALL files ---
add_library(${PROJECT_NAME} STATIC
    ${categorized_src_files}
    ${other_src_files}
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
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        # Libs included in your engine's .h files
        libglew_static
        glfw
        glm::glm
        imgui          # (If you include imgui.h in your headers)

    PRIVATE
        # Libs used only in your engine's .cpp files
        stb
        freetype
        reflect
        rapidjson
        CrashoutsAudio
)
link_fmod_to(${PROJECT_NAME})

# Apply common include directories
target_include_directories(${PROJECT_NAME} 
PUBLIC
    ${project_dir}/include
    ${CMAKE_SOURCE_DIR}/third_party/dotnet/include
PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/glm                 # Added GLM include
    ${_FMOD_INC}                                # Added FMOD include
)

# Tell the compiler to export symbols using DLL_API_EXPORT
target_compile_definitions(${PROJECT_NAME} PRIVATE DLL_API_EXPORT)

# Ensure exception handling is on
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
endif()

#[[
# Copy .NET runtime dependencies after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/third_party/dotnet/bin"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMENT "Copying .NET Core runtime dependencies..."
)
]]

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

# .NET CoreCLR Hosting for C# scripting

# Add Windows libraries for dynamic loading
target_link_libraries(${PROJECT_NAME} PRIVATE shlwapi.lib)

# Ensure your CoreCLR host headers are included
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/dotnet/include
)
