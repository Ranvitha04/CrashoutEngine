include(FetchContent)
function(disable_warnings_for target)
    if (MSVC)
        # Get existing compile options for the target
        get_target_property(existing_compile_options ${target} COMPILE_OPTIONS)

        if (existing_compile_options)
            # Remove any /W0�/W4 already present
            string(REGEX REPLACE "/W[0-4]" "" existing_compile_options "${existing_compile_options}")
            
            # FIX: Use semicolon to separate the flags in the list
            set_target_properties(${target} PROPERTIES COMPILE_OPTIONS "${existing_compile_options};/W0;/analyze-")
        else()
            # FIX: Use semicolon to separate the flags in the list
            set_target_properties(${target} PROPERTIES COMPILE_OPTIONS "/W0;/analyze-")
        endif()
    else()
        target_compile_options(${target} PRIVATE -w)
    endif()
endfunction()

# -------------------- GLFW --------------------
macro(import_glfw)
    if(NOT TARGET glfw)
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.8
        )
        FetchContent_MakeAvailable(glfw)
    endif()
endmacro()

# -------------------- GLM (header-only) --------------------
macro(import_glm)
    if(NOT TARGET glm::glm)
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(glm)
        # glm already provides glm::glm target, no manual include needed
    endif()
endmacro()

# -------------------- GLEW --------------------
macro(import_glew)
    if(NOT TARGET libglew_static)
        FetchContent_Declare(
            glew
            GIT_REPOSITORY https://github.com/omniavinco/glew-cmake.git
            GIT_TAG glew-cmake-2.2.0
        )
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glew)
        # glew-cmake defines libglew_static target
    endif()
endmacro()

# -------------------- ImGui --------------------
macro(import_imgui)
    if(NOT TARGET imgui)
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG docking
        )
        FetchContent_Populate(imgui)

        set(IMGUI_SOURCES
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )

        add_library(imgui STATIC ${IMGUI_SOURCES})

        disable_warnings_for(imgui)

        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC glfw libglew_static)
    endif()
endmacro()

# -------------------- ImGuizmo -----------------------------
macro(import_imguizmo)
    if(NOT TARGET imguizmo)
        FetchContent_Declare(
            imguizmo
            GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
            GIT_TAG master
        )
        FetchContent_GetProperties(imguizmo)
        if(NOT imguizmo_POPULATED)
            FetchContent_Populate(imguizmo)
        endif()

        add_library(imguizmo STATIC ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp)

        disable_warnings_for(imguizmo)

        target_include_directories(imguizmo PUBLIC ${imguizmo_SOURCE_DIR})

        # ImGuizmo depends on Dear ImGui headers/symbols
        target_link_libraries(imguizmo PUBLIC imgui)
    endif()
endmacro()

# -------------------- stb (header-only) --------------------
macro(import_stb)
    if(NOT TARGET stb)
        FetchContent_Declare(
            stb
            GIT_REPOSITORY https://github.com/nothings/stb.git
            GIT_TAG master
        )
        FetchContent_Populate(stb)

        if (TARGET stb)
            disable_warnings_for(stb)
        endif()

        add_library(stb INTERFACE)
        target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
    endif()
endmacro()

# -------------------- freetype --------------------
macro(import_freetype)
    if(NOT TARGET freetype)
        FetchContent_Declare(
            freetype
            GIT_REPOSITORY https://github.com/freetype/freetype.git
            GIT_TAG VER-2-13-2
        )
        FetchContent_MakeAvailable(freetype)
        if (TARGET freetype)
            disable_warnings_for(freetype)
        endif()

        # --- THIS IS THE CORRECT WAY ---
        # Get the include directory property FROM the target
        get_target_property(FREETYPE_INCLUDE_DIRS freetype INTERFACE_INCLUDE_DIRECTORIES)
        
        # Set your variable from this (but use a better name)
        set(_FREETYPE_INC ${FREETYPE_INCLUDE_DIRS})
        
    endif()
endmacro()

# -------------------- reflect.cpp --------------------
macro(import_reflect)
    if(NOT TARGET reflect)
        FetchContent_Declare(
            reflect
            GIT_REPOSITORY https://github.com/getml/reflect-cpp.git
            GIT_TAG v0.20.0
        )
        FetchContent_Populate(reflect)
        if (TARGET reflect)
            disable_warnings_for(reflect)
        endif()
        add_library(reflect INTERFACE)
        target_include_directories(reflect SYSTEM INTERFACE ${reflect_SOURCE_DIR}/include)
    endif()
endmacro()

# -------------------- rapidjson (header-only) --------------------
macro(import_rapidjson)
    if(NOT TARGET rapidjson)
        FetchContent_Declare(
            rapidjson
            GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
            GIT_TAG v1.1.0
        )
        FetchContent_Populate(rapidjson)

        add_library(rapidjson INTERFACE)
        target_include_directories(rapidjson SYSTEM INTERFACE ${rapidjson_SOURCE_DIR}/include)

        # SUPRESS DEPRECATED WARNINGS FOR RAPIDJSON
        if(MSVC)
            # Suppress C++17 std::iterator deprecation warning (STL4015)
            # caused by rapidjson v1.1.0, which uses a deprecated base class.
            # add_compile_definitions(_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
            target_compile_options(rapidjson INTERFACE
                $<$<COMPILE_LANGUAGE:CXX>:/wd26495 /wd33010>
            )
        endif()

    endif()
endmacro()
#[[
# FMOD paths
set(_FMOD          "${CMAKE_SOURCE_DIR}/third_party/fmod")
set(_FMOD_INC      "${_FMOD}/include")
set(_FMOD_LIBDIR   "${_FMOD}/lib/x64")
set(_FMOD_BINDIR   "${_FMOD}/bin/x64")
# Core
find_library(FMOD_CORE_LIB
    NAMES fmod_vc fmod fmodL_vc
    PATHS ${_FMOD_LIBDIR}
    NO_DEFAULT_PATH
)

# Studio
find_library(FMOD_STUDIO_LIB
    NAMES fmodstudio_vc fmodstudio
    PATHS ${_FMOD_LIBDIR}
    NO_DEFAULT_PATH
)

if(NOT FMOD_CORE_LIB)
    message(FATAL_ERROR "[FMOD] Could not find FMOD Core lib in ${_FMOD_LIBDIR}")
endif()

if(NOT FMOD_STUDIO_LIB)
    message(FATAL_ERROR "[FMOD] Could not find FMOD Studio lib in ${_FMOD_LIBDIR}")
endif()

add_library(FMOD::Core SHARED IMPORTED)
set_target_properties(FMOD::Core PROPERTIES
    IMPORTED_IMPLIB               "${FMOD_CORE_LIB}"
    IMPORTED_LOCATION             "${_FMOD_BINDIR}/fmod.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${_FMOD_INC}"
)

add_library(FMOD::Studio SHARED IMPORTED)
set_target_properties(FMOD::Studio PROPERTIES
    IMPORTED_IMPLIB               "${FMOD_STUDIO_LIB}"
    IMPORTED_LOCATION             "${_FMOD_BINDIR}/fmodstudio.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${_FMOD_INC}"
)
]]
# Expected repo layout:
# third_party/fmod/
#   include/            fmod.hpp, fmod_studio.hpp
#   lib/x64/            fmod_vc.lib, fmodstudio_vc.lib
#   bin/x64/            fmod.dll (or fmodL.dll),fmodstudio.dll

set(_FMOD          "${CMAKE_SOURCE_DIR}/third_party/fmod")
set(_FMOD_INC      "${_FMOD}/include")
set(_FMOD_LIBDIR   "${_FMOD}/lib/x64")
set(_FMOD_BINDIR   "${_FMOD}/bin/x64")


foreach(_need
  "${_FMOD_INC}/fmod.hpp"
  "${_FMOD_INC}/fmod_studio.hpp"
  "${_FMOD_LIBDIR}/fmod_vc.lib"
  "${_FMOD_LIBDIR}/fmodstudio_vc.lib"
  "${_FMOD_BINDIR}/fmod.dll"          
  "${_FMOD_BINDIR}/fmodstudio.dll")
  if(NOT EXISTS "${_need}")
    message(FATAL_ERROR "[FMOD] Missing: ${_need}")
  endif()
endforeach()


add_library(FMOD::Core SHARED IMPORTED)
set_target_properties(FMOD::Core PROPERTIES
  IMPORTED_IMPLIB               "${_FMOD_LIBDIR}/fmod_vc.lib"
  IMPORTED_LOCATION             "${_FMOD_BINDIR}/fmod.dll"#target_compile_features(CrashoutsAudio PUBLIC cxx_std_17)
  IMPORTED_LOCATION_DEBUG       "${_FMOD_BINDIR}/fmodL.dll"  
  INTERFACE_INCLUDE_DIRECTORIES "${_FMOD_INC}"
)

add_library(FMOD::Studio SHARED IMPORTED)
set_target_properties(FMOD::Studio PROPERTIES
  IMPORTED_IMPLIB               "${_FMOD_LIBDIR}/fmodstudio_vc.lib"
  IMPORTED_LOCATION             "${_FMOD_BINDIR}/fmodstudio.dll"
  INTERFACE_INCLUDE_DIRECTORIES "${_FMOD_INC}"
)


function(link_fmod_to target)
  target_link_libraries(${target} PRIVATE FMOD::Core FMOD::Studio)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_FMOD_BINDIR}/fmod.dll"        "$<TARGET_FILE_DIR:${target}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_FMOD_BINDIR}/fmodL.dll"       "$<TARGET_FILE_DIR:${target}>"  
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_FMOD_BINDIR}/fmodstudio.dll" "$<TARGET_FILE_DIR:${target}>"
  )
endfunction()


function(copy_fmod_banks target banks_dir)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/assets/audio/banks"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${banks_dir}" "$<TARGET_FILE_DIR:${target}>/assets/audio/banks"
  )
  endfunction()

set(CA_INC "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include")
set(CA_SRC "${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/src")


add_library(CrashoutsAudio STATIC

  "${CA_SRC}/audioSystem.cpp"         
)
target_compile_features(CrashoutsAudio PUBLIC cxx_std_17)
target_include_directories(CrashoutsAudio PUBLIC "${CA_INC}")
target_link_libraries(CrashoutsAudio PUBLIC FMOD::Core FMOD::Studio)

# -------------------- MONO --------------------
set(_MONO "${CMAKE_SOURCE_DIR}/third_party/Mono")
set(_MONO_INC "${_MONO}/include/mono-2.0")
set(_MONO_LIBDIR "${_MONO}/lib")
set(_MONO_BINDIR "${_MONO}/bin")
set(_MONO_ETCDIR "${_MONO}/etc")

# --- Verify required files ---
foreach(_need
    "${_MONO_INC}/mono/jit/jit.h"
    "${_MONO_LIBDIR}/mono-2.0-sgen.lib"
    "${_MONO_BINDIR}/mono-2.0-sgen.dll")
    if(NOT EXISTS "${_need}")
        message(FATAL_ERROR "[Mono] Missing: ${_need}")
    endif()
endforeach()

# --- Create imported targets ---
add_library(Mono::Core SHARED IMPORTED)
set_target_properties(Mono::Core PROPERTIES
    IMPORTED_IMPLIB               "${_MONO_LIBDIR}/mono-2.0-sgen.lib"
    IMPORTED_LOCATION             "${_MONO_BINDIR}/mono-2.0-sgen.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${_MONO_INC}"
)

# --- Helper function to link Mono + copy runtime ---
function(link_mono_to target)
    message(STATUS "[Mono] Linking runtime to target: ${target}")

    # Create imported Mono target if missing
    if(NOT TARGET Mono::Core)
        add_library(Mono::Core SHARED IMPORTED)
        set_target_properties(Mono::Core PROPERTIES
            IMPORTED_IMPLIB               "${_MONO_LIBDIR}/mono-2.0-sgen.lib"
            IMPORTED_LOCATION             "${_MONO_BINDIR}/mono-2.0-sgen.dll"
            INTERFACE_INCLUDE_DIRECTORIES "${_MONO_INC}"
        )
    endif()

    # Link library + include path
    target_link_libraries(${target} PRIVATE Mono::Core)
    target_include_directories(${target} PRIVATE "${_MONO_INC}")

    # Post-build copy (ensures runtime DLL + etc)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_MONO_BINDIR}/mono-2.0-sgen.dll" "$<TARGET_FILE_DIR:${target}>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${_MONO_ETCDIR}" "$<TARGET_FILE_DIR:${target}>/third_party/Mono/etc"
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${_MONO_LIBDIR}" "$<TARGET_FILE_DIR:${target}>/third_party/Mono/lib"
    )

    message(STATUS "[Mono] Linked ${target} with includes at: ${_MONO_INC}")
endfunction()

# (optional) log once so we know it ran
message(STATUS "[CrashoutsAudio] src='${CA_SRC}/audioSystem.cpp'")
# Macro to import all dependencies
macro(importDependencies)
    message(STATUS "Starting to import dependencies...")

    import_glfw()
    message(STATUS "GLFW imported successfully.")

    import_glm()
    message(STATUS "GLM imported successfully.")

    import_glew()
    message(STATUS "GLEW imported successfully.")

    import_imgui()
    message(STATUS "ImGui imported successfully.")

    import_imguizmo()
    message(STATUS "ImGuizmo imported successfully.")

    import_stb()
    message(STATUS "stb imported successfully.")

    import_freetype()
    message(STATUS "FreeType imported successfully.")

    import_reflect()
    message(STATUS "reflect.cpp imported successfully.")

    import_rapidjson()
    message(STATUS "rapidjson imported successfully.")

    message(STATUS "All dependencies have been imported successfully.")
endmacro()

# -------------------- FFmpeg --------------------
set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/third_party/ffmpeg")
set(FFMPEG_INCLUDE_DIR "${FFMPEG_ROOT}/include")
set(FFMPEG_LIBRARY_DIR "${FFMPEG_ROOT}/lib")
set(FFMPEG_BINARY_DIR  "${FFMPEG_ROOT}/bin")

set(FFMPEG_LIBS
    avcodec
    avdevice
    avfilter
    avformat
    avutil
    swresample
    swscale
)

function(link_ffmpeg_to target)
    if(NOT EXISTS "${FFMPEG_ROOT}")
        message(WARNING "FFmpeg root directory not found at: ${FFMPEG_ROOT}. Video features may be disabled.")
        return()
    endif()

    target_include_directories(${target} PRIVATE ${FFMPEG_INCLUDE_DIR})
    
    # Enable the flag for code wrapped in #ifdef USE_FFMPEG
    target_compile_definitions(${target} PRIVATE USE_FFMPEG)

    foreach(LIB_NAME ${FFMPEG_LIBS})
        find_library(FFMPEG_${LIB_NAME}_LIB
            NAMES ${LIB_NAME}
            PATHS ${FFMPEG_LIBRARY_DIR}
            NO_DEFAULT_PATH
        )

        if(FFMPEG_${LIB_NAME}_LIB)
            target_link_libraries(${target} PRIVATE ${FFMPEG_${LIB_NAME}_LIB})
        else()
            message(WARNING "Could not find FFmpeg library: ${LIB_NAME} in ${FFMPEG_LIBRARY_DIR}")
        endif()
    endforeach()

    # Copy DLLs to the executable directory (post-build)
    file(GLOB FFMPEG_DLLS "${FFMPEG_BINARY_DIR}/*.dll")
    
    foreach(DLL ${FFMPEG_DLLS})
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL}"
            "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying FFmpeg DLL: ${DLL} to output directory"
        )
    endforeach()
endfunction()
