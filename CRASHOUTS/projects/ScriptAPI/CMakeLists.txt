project(ScriptAPI LANGUAGES CXX)
set(project_dir ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "Configuring project: ${PROJECT_NAME}")

file(GLOB_RECURSE all_src_files     "${project_dir}/src/*.cpp")
file(GLOB_RECURSE all_include_files "${project_dir}/include/*.h" "${project_dir}/include/*.hpp")

add_library(${PROJECT_NAME} SHARED
    ${all_src_files}
    ${all_include_files})

if (MSVC)
    # Always compile as C++/CLI
    target_compile_options(${PROJECT_NAME} PRIVATE /clr /EHa)

    # Disable runtime checks added by VS in CLR-Debug configuration
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MSVC_RUNTIME_CHECKS "OFF"
    )

    # Disable runtime checks in Debug-like builds when /clr is active
    if (MSVC AND (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "CLR-Debug"))
        target_compile_options(${PROJECT_NAME} PRIVATE /RTC1- /EHs-)
    endif()
endif()


# Link to the native engine
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        CrashoutEngine       # links to your core engine
)

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