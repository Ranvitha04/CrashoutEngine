include(FetchContent)

# Macro to import GLFW
macro(import_glfw)
    if(NOT TARGET glfw)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.8
        )
        if(NOT glfw_POPULATED)
            set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            FetchContent_Populate(glfw)
        endif()

        add_subdirectory(${glfw_SOURCE_DIR})
        include_directories(${GLFW_SOURCE_DIR}/include)
    endif()
endmacro()

# Macro to import glm
macro(import_glm)
    if(NOT TARGET glm)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG master
        )
        FetchContent_MakeAvailable(glm)

        include_directories(${glm_SOURCE_DIR})
    endif()
endmacro()

# Macro to import glew
macro(import_glew)
    if(NOT TARGET glew_s)  # Guard to prevent multiple inclusion
        include(FetchContent)

        FetchContent_Declare(
            glew
            GIT_REPOSITORY https://github.com/omniavinco/glew-cmake.git
            GIT_TAG master  # Or use a specific commit or tag if preferred
        )

        # Disable shared builds if needed
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)
        FetchContent_MakeAvailable(glew)

        # Optional: include directories if needed manually (though target_link_libraries handles it)
        include_directories(${glew_SOURCE_DIR}/include)
    endif()
endmacro()

# Macro to import ImGui - DOCKING version
macro(import_imgui)
    if(NOT TARGET imgui)  # Guard to prevent multiple inclusion
        FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG docking
        )
        if(NOT imgui_POPULATED)
            FetchContent_Populate(imgui)
        endif()

        # Create ImGui library manually since it doesn't have CMakeLists.txt
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
        target_include_directories(imgui PUBLIC 
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC glfw libglew_static)
    endif()
endmacro()

# Expected repo layout:
# third_party/fmod/
#   include/            fmod.hpp, fmod_studio.hpp
#   lib/x64/            fmod_vc.lib, fmodstudio_vc.lib
#   bin/x64/            fmod.dll (or fmodL.dll), fmodstudio.dll

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
  IMPORTED_LOCATION             "${_FMOD_BINDIR}/fmod.dll"target_compile_features(CrashoutsAudio PUBLIC cxx_std_17)
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
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${banks_dir}" "$<TARGET_FILE_DIR:${target}>/assets/audio/banks"
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

# (optional) log once so we know it ran
message(STATUS "[CrashoutsAudio] src='${CA_SRC}/audioSystem.cpp'")


# Macro to import all dependencies
macro(importDependencies)
    message(STATUS "Starting to import dependencies...")
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE "${CMAKE_SOURCE_DIR}/third_party/stb")
    target_include_directories(stb INTERFACE "${CMAKE_BINARY_DIR}/_deps/stb-src")
    message(STATUS "Importing GLFW...")
    import_glfw()
    message(STATUS "GLFW imported successfully.")

    message(STATUS "Importing GLM...")
    import_glm()
    message(STATUS "GLM imported successfully.")

    message(STATUS "Importing GLEW...")
    import_glew()
    message(STATUS "GLEW imported successfully.")

    message(STATUS "Importing ImGui...")
    import_imgui()
    message(STATUS "ImGui imported successfully.")

    message(STATUS "All dependencies have been imported successfully.")
endmacro()