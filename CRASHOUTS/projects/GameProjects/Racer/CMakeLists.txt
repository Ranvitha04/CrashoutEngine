project(Racer)
set(project_dir ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE src_files "${project_dir}/src/*.cpp")
file(GLOB_RECURSE inc_files "${project_dir}/include/*.h")

# ======================================================
# TARGET 1: The Standalone Game
# ======================================================
add_executable(Racer WIN32 ${src_files} ${inc_files})
# Link to the standard engine
target_link_libraries(Racer PRIVATE CrashoutEngine)
target_include_directories(Racer PRIVATE 
    "${project_dir}/include" 
    "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include"
)
set_target_properties(Racer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Racer>")

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
set(CSPROJ_PATH "${project_dir}/scripts/Racer.csproj")
add_custom_target(Racer_Scripts ALL
    COMMAND dotnet build "${CSPROJ_PATH}" -c $<CONFIG>
    WORKING_DIRECTORY "${project_dir}/scripts"
)
add_dependencies(Racer Racer_Scripts)
add_dependencies(CrashoutEditor Racer_Scripts)

# --- Asset Sync (Shared) ---
add_custom_command(TARGET Racer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${project_dir}/assets" "$<TARGET_FILE_DIR:Racer>/assets"
)
add_custom_command(TARGET CrashoutEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${project_dir}/assets" "$<TARGET_FILE_DIR:CrashoutEditor>/assets"
)

# --- NEW: Copy Engine Core Assets for Standalone ---
add_custom_command(TARGET Racer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/core-assets" "$<TARGET_FILE_DIR:Racer>/core-assets"
)

# Copy compiled C# Script DLL
add_custom_command(TARGET Racer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/build/x64-$<CONFIG>/scripts/Racer" "$<TARGET_FILE_DIR:Racer>/scripts"
)

# --- NEW: Copy Runtime DLLs needed for standalone ---
add_custom_command(TARGET Racer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/third_party/fmod/bin/x64/fmod.dll" "$<TARGET_FILE_DIR:Racer>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/third_party/fmod/bin/x64/fmodstudio.dll" "$<TARGET_FILE_DIR:Racer>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/third_party/Mono/bin/mono-2.0-sgen.dll" "$<TARGET_FILE_DIR:Racer>"
)