include(FetchContent)

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
            GIT_TAG master
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
        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC glfw libglew_static)
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

        add_library(reflect INTERFACE)
        target_include_directories(reflect INTERFACE ${reflect_SOURCE_DIR}/include)
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
        target_include_directories(rapidjson INTERFACE ${rapidjson_SOURCE_DIR}/include)
    endif()
endmacro()

# -------------------- Import all --------------------
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
