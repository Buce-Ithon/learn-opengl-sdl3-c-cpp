cmake_minimum_required(VERSION 3.20)
project(learn-opengl-sdl3 
VERSION 1.0.0 
LANGUAGES C CXX
DESCRIPTION "A modern OpenGL (SDL3 version) graphics rendering project."
)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

#set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_GENERATOR MATCHES "Makefiles|Ninja")
	if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
		add_custom_target(
			CopyCompileCommands ALL
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				${CMAKE_BINARY_DIR}/compile_commands.json
				${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
			DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
			COMMENT "Copy compile_commands.json to the project source directory."
		)
	endif()
endif()

add_subdirectory(vendor)
add_subdirectory(src)