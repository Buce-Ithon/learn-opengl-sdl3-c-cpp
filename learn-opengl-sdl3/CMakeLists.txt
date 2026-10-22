aux_source_directory(src LOD_SOURCE_FILES)
message("LOD_SOURCE_FILES : ${LOD_SOURCE_FILES}")
file(GLOB_RECURSE LOD_HEADER_FILES include/*.h)
message("LOD_HEADER_FILES : ${LOD_HEADER_FILES}")

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${LOD_SOURCE_FILES} ${LOD_HEADER_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE vendor)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

if (WIN32)
    target_sources(${PROJECT_NAME} PRIVATE resources/windows_resources.rc)
endif()