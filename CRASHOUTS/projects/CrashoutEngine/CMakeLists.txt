# Project name for the Engine.
project(CrashoutEngine)
set(project_dir ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "Configuring project: ${PROJECT_NAME}")

# --- 1. GLOB CATEGORIES ---
file(GLOB_RECURSE manager_src_files      "${project_dir}/src/*manager.*")
file(GLOB_RECURSE manager_include_files  "${project_dir}/include/*manager.*")
file(GLOB_RECURSE component_src_files    "${project_dir}/src/*component.*")
file(GLOB_RECURSE component_include_files "${project_dir}/include/*component.*")
file(GLOB_RECURSE scene_src_files        "${project_dir}/src/*scene.*")
file(GLOB_RECURSE scene_include_files    "${project_dir}/include/*scene.*")
file(GLOB_RECURSE resource_src_files     "${project_dir}/src/*asset.*")
file(GLOB_RECURSE resource_include_files "${project_dir}/include/*asset.*")
file(GLOB_RECURSE debug_src_files        "${project_dir}/src/debug*.*")
file(GLOB_RECURSE debug_include_files    "${project_dir}/include/debug*.*")
file(GLOB_RECURSE binding_src_files      "${project_dir}/src/*bindings.*")
file(GLOB_RECURSE binding_include_files  "${project_dir}/include/*bindings.*")

file(GLOB_RECURSE message_include_files 
    "${project_dir}/include/messages.h"
    "${project_dir}/include/observable.h"
    "${project_dir}/include/observer.h")

file(GLOB_RECURSE math_include_files 
    "${project_dir}/include/CEMath.h"
    "${project_dir}/include/CEMtx33.h"
    "${project_dir}/include/CETypes.h"
    "${project_dir}/include/CEColor.h"
    "${project_dir}/include/CEVec2.h"
    "${project_dir}/include/CEVec3.h")

file(GLOB_RECURSE all_src_files      "${project_dir}/src/*.cpp")
file(GLOB_RECURSE all_include_files  "${project_dir}/include/*.h" "${project_dir}/include/*.hpp")

set(categorized_src_files ${manager_src_files} ${component_src_files} ${scene_src_files} ${resource_src_files} ${debug_src_files} ${binding_src_files})
set(categorized_include_files ${manager_include_files} ${component_include_files} ${scene_include_files} ${resource_include_files} ${debug_include_files} ${message_include_files} ${math_include_files} ${binding_include_files})

list(REMOVE_ITEM all_src_files      ${categorized_src_files})
list(REMOVE_ITEM all_include_files  ${categorized_include_files})
set(other_src_files      ${all_src_files})
set(other_include_files  ${all_include_files})

set(ENGINE_COMMON_FILES 
    ${categorized_src_files} 
    ${categorized_include_files} 
    ${other_src_files} 
    ${other_include_files}
    "${project_dir}/include/engine_pch.h"
    "${project_dir}/src/particleSystem.cpp"
    "${project_dir}/include/particleSystem.h"
)

# ============================================================================
# 2. DUAL ENGINE LIBRARIES (The Fix)
# ============================================================================

# Library A: The Clean Engine (No Editor)
add_library(CrashoutEngine STATIC ${ENGINE_COMMON_FILES})
target_compile_definitions(CrashoutEngine PUBLIC USE_IMGUI_EDITOR=0)

# Library B: The Editor Engine (Includes ImGui logic)
add_library(CrashoutEngine_Editor STATIC ${ENGINE_COMMON_FILES})
target_compile_definitions(CrashoutEngine_Editor PUBLIC USE_IMGUI_EDITOR=1)

# Apply all includes and links to BOTH libraries cleanly
set(ENGINE_LIBRARIES CrashoutEngine CrashoutEngine_Editor)

foreach(TARGET_LIB ${ENGINE_LIBRARIES})
    target_include_directories(${TARGET_LIB} PUBLIC 
        "${project_dir}/include" 
        ${_MONO_INC}
        PRIVATE 
        ${CMAKE_SOURCE_DIR}/lib/glm
        ${_FMOD_INC}
        ${_FREETYPE_INC}
    )

    target_compile_definitions(${TARGET_LIB} PUBLIC 
        ENGINE_CORE_ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/core-assets/"
    )

    target_precompile_headers(${TARGET_LIB} PRIVATE "include/engine_pch.h")

    target_link_libraries(${TARGET_LIB} PUBLIC 
        libglew_static glfw glm::glm imgui 
        PRIVATE stb freetype reflect rapidjson CrashoutsAudio imguizmo
    )

    link_fmod_to(${TARGET_LIB})
    link_mono_to(${TARGET_LIB})
    link_ffmpeg_to(${TARGET_LIB})
endforeach()