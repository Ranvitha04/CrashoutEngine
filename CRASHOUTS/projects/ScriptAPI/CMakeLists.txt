project(ScriptAPI LANGUAGES CXX)
set(project_dir ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "Configuring project: ${PROJECT_NAME}")

file(GLOB_RECURSE all_src_files     "${project_dir}/src/*.cpp")
file(GLOB_RECURSE all_include_files "${project_dir}/include/*.h" "${project_dir}/include/*.hpp")

add_library(${PROJECT_NAME} SHARED
    ${all_src_files}
    ${all_include_files})

# Enable C++/CLI support (CLR)
if (MSVC)
    # Remove conflicting MSVC flags added by CMake
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/EHs" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTCs" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/RTCsu" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

    # Apply correct C++/CLI compile options
    target_compile_options(${PROJECT_NAME} PRIVATE /clr /EHa /Zc:twoPhase- /wd4624 /wd4635 /wd4946)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4005)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_EnableRTC "false")
# Link to the native engine
target_link_libraries(${PROJECT_NAME} PRIVATE
    CrashoutEngine
    MSCOREE.lib
)

# Add .NET Framework SDK lib path for MSCOREE.lib
if (MSVC)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(NETFX_PATH "$ENV{ProgramFiles\(x86\)}/Windows Kits/NETFXSDK/4.8/Lib/um/x64")
    else()
        set(NETFX_PATH "$ENV{ProgramFiles\(x86\)}/Windows Kits/NETFXSDK/4.8/Lib/um/x86")
    endif()
    link_directories("${NETFX_PATH}")
endif()

# Include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${project_dir}/include
        ${CMAKE_SOURCE_DIR}/projects/CrashoutEngine/include
)

# Output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

#  MSVC-specific C++/CLI setup
if (MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_USER_PROPS "${CMAKE_CURRENT_LIST_DIR}/scriptapi_clr.props"
    )
endif()

# Copy ScriptAPI.dll to the engine output directory after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "$<TARGET_FILE_DIR:CrashoutEngine>/ScriptAPI.dll"
    COMMENT "Copying ScriptAPI.dll to engine output..."
)