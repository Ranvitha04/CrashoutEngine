project(Descent)
set(project_dir ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE src_files "${project_dir}/src/*.cpp")
file(GLOB_RECURSE inc_files "${project_dir}/include/*.h")

# ======================================================
# TARGET 1: The Standalone Game
# ======================================================
add_executable(Descent WIN32 ${src_files} ${inc_files})
# Link to the standard engine
target_link_libraries(Descent PRIVATE CrashoutEngine)
target_include_directories(Descent PRIVATE 
    "${project_dir}/include" 
    "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include"
)
set_target_properties(Descent PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Descent>")

# ======================================================
# TARGET 2: The Engine Editor
# ======================================================
add_executable(CrashoutEditor WIN32 ${src_files} ${inc_files})
# Link specifically to the Editor engine
target_link_libraries(CrashoutEditor PRIVATE CrashoutEngine_Editor)
target_include_directories(CrashoutEditor PRIVATE 
    "${project_dir}/include" 
    "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include"
)
set_target_properties(CrashoutEditor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:CrashoutEditor>")

# --- C# Build Trigger (Shared) ---
set(CSPROJ_PATH "${project_dir}/scripts/Descent.csproj")
add_custom_target(Descent_Scripts ALL
    COMMAND dotnet build "${CSPROJ_PATH}" -c $<CONFIG>
    WORKING_DIRECTORY "${project_dir}/scripts"
)
add_dependencies(Descent Descent_Scripts)
add_dependencies(CrashoutEditor Descent_Scripts)

# --- Asset Sync (Shared) ---
add_custom_command(TARGET Descent POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${project_dir}/assets" "$<TARGET_FILE_DIR:Descent>/assets"
)
add_custom_command(TARGET CrashoutEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${project_dir}/assets" "$<TARGET_FILE_DIR:CrashoutEditor>/assets"
)