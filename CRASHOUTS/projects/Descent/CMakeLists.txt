#projects/Descent/CMakeLists.txt

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

# Ensure scripts build before the game
if (TARGET ManagedScripts)
    add_dependencies(Descent ManagedScripts)
endif()

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

if (TARGET ManagedScripts)
    add_dependencies(CrashoutEditor ManagedScripts)
endif()
