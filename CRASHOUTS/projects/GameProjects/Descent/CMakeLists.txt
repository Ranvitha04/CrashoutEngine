#projects/GameProjects/Descent/CMakeLists.txt

project(Descent)

set(project_dir ${CMAKE_CURRENT_LIST_DIR})

message(STATUS "Configuring project: ${PROJECT_NAME}")

# ------------------------------------------------------------
# ICON (Windows only)
# ------------------------------------------------------------
if (WIN32)
    set(DESCENT_ICON "${project_dir}/DescentResources.rc")
endif()

# ------------------------------------------------------------
# SOURCE / HEADER GLOBS
# ------------------------------------------------------------
file(GLOB_RECURSE all_src_files     "${project_dir}/src/*.cpp")
file(GLOB_RECURSE all_include_files "${project_dir}/include/*.h" "${project_dir}/include/*.hpp")

set(other_src_files     ${all_src_files})
set(other_include_files ${all_include_files})

# ------------------------------------------------------------
# DESCENT GAME BUILD (Descent.exe)
# ------------------------------------------------------------
add_executable(${PROJECT_NAME} WIN32
    ${DESCENT_ICON}
    ${other_src_files}
    ${other_include_files}
    "${project_dir}/include/pch.h"
)

# PCH
target_precompile_headers(${PROJECT_NAME} PRIVATE "include/pch.h")

# Organize in Visual Studio filters
source_group("src"     FILES ${other_src_files})
source_group("include" FILES ${other_include_files})

# Link runtime engine
target_link_libraries(${PROJECT_NAME}
    PRIVATE CrashoutEngine
)

link_mono_to(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${project_dir}/include
)

set_property(TARGET ${PROJECT_NAME}
    PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# Copy assets
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    "${project_dir}/assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

# ------------------------------------------------------------
# EDITOR BUILD (CrashoutEditor.exe)
# ------------------------------------------------------------
add_executable(CrashoutEditor WIN32
    ${DESCENT_ICON}
    ${other_src_files}
    ${other_include_files}
    "${project_dir}/include/pch.h"
)

set_target_properties(CrashoutEditor
    PROPERTIES OUTPUT_NAME "CrashoutEditor"
)

target_precompile_headers(CrashoutEditor PRIVATE "include/pch.h")

target_link_libraries(CrashoutEditor
    PRIVATE CrashoutEngine.Editor
)

link_mono_to(CrashoutEditor)
link_fmod_to(CrashoutEditor)

target_include_directories(CrashoutEditor PRIVATE
    ${project_dir}/include
    ${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include
)

set_property(TARGET CrashoutEditor
    PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:CrashoutEditor>"
)

# assets for editor
add_custom_command(TARGET CrashoutEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${project_dir}/assets"
        "$<TARGET_FILE_DIR:CrashoutEditor>/assets"
)

# Build Descent.csproj automatically when we build the Descent game
add_custom_target(DescentScripts ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Building Descent Scripts..."
    COMMAND dotnet build "${CMAKE_CURRENT_SOURCE_DIR}/scripts/Descent.csproj" -c $<CONFIG>
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scripts"
    COMMENT "Building C# scripts for Descent"
)

# Ensure the output directory exists in the build folder
add_custom_command(
    TARGET DescentScripts POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/scripts"
    COMMENT "Ensuring runtime scripts folder exists"
)

# Make BOTH your Game and Editor wait for the C# scripts to compile before they launch
add_dependencies(Descent DescentScripts)
add_dependencies(CrashoutEditor DescentScripts)
