
cmake_minimum_required(VERSION 3.20)
project(Testing)

set(project_dir ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE all_src_files     "${project_dir}/src/*.cpp")
file(GLOB_RECURSE all_include_files "${project_dir}/include/*.h")

# ======================================================
# TARGET 1: Standalone Game
# ======================================================
add_executable(${PROJECT_NAME} WIN32 ${all_src_files} ${all_include_files})

# Link to the CLEAN engine library (USE_IMGUI_EDITOR=0)
target_link_libraries(${PROJECT_NAME} PRIVATE CrashoutEngine)

target_include_directories(${PROJECT_NAME} PRIVATE 
    "${project_dir}/include"
    "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include"
)

# Fix for Visual Studio F5: Set Working Directory to output folder
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

# ======================================================
# TARGET 2: Engine Editor
# ======================================================
add_executable(CrashoutEditor WIN32 ${all_src_files} ${all_include_files})

# Link to the EDITOR engine library (USE_IMGUI_EDITOR=1)
target_link_libraries(CrashoutEditor PRIVATE CrashoutEngine_Editor)

target_include_directories(CrashoutEditor PRIVATE 
    "${project_dir}/include"
    "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include"
)

# Fix for Visual Studio F5: Set Working Directory to output folder
set_target_properties(CrashoutEditor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:CrashoutEditor>")

# ======================================================
# Shared Build Steps (C# Scripts & Asset Sync)
# ======================================================
set(CSPROJ_PATH "${project_dir}/scripts/Testing.csproj")
add_custom_target(Testing_Scripts ALL
    COMMAND dotnet build "${CSPROJ_PATH}" -c $<CONFIG>
    WORKING_DIRECTORY "${project_dir}/scripts"
)
add_dependencies(${PROJECT_NAME} Testing_Scripts)
add_dependencies(CrashoutEditor Testing_Scripts)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${project_dir}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)
add_custom_command(TARGET CrashoutEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${project_dir}/assets" "$<TARGET_FILE_DIR:CrashoutEditor>/assets"
)
