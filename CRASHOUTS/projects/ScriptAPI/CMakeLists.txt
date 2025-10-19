project(ScriptAPI LANGUAGES CXX)
set(project_dir ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "Configuring project: ${PROJECT_NAME}")

file(GLOB_RECURSE all_src_files     "${project_dir}/src/*.cpp")
file(GLOB_RECURSE all_include_files "${project_dir}/include/*.h" "${project_dir}/include/*.hpp")

add_library(${PROJECT_NAME} SHARED
    ${all_src_files}
    ${all_include_files})

# Link to the native engine
target_link_libraries(${PROJECT_NAME} PRIVATE CrashoutEngine)

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

# ---------------------------------------------------------------
#  MSVC-specific C++/CLI setup
# ---------------------------------------------------------------
if (MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_USER_PROPS "${CMAKE_CURRENT_LIST_DIR}/scriptapi_clr.props"
    )
endif()
