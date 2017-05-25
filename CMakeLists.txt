cmake_minimum_required(VERSION 3.5)

if (NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio .+")
	message(FATAL_ERROR "The only supported compiler is MSVC.")
endif ()

if ("${CMAKE_GENERATOR}" MATCHES "(Win64|ARM)")
	message(FATAL_ERROR "ARM and x64 are not supported.")
endif()

set (CMAKE_CXX_STANDARD 11)

project(THRotator)

set(BOOST_INCLUDE_DIR "" CACHE PATH "boost header file directory")
set(BOOST_LIB_DIR "" CACHE PATH "boost library directory")

add_subdirectory(d3d9)
add_subdirectory(d3d8)
add_subdirectory(localization_en-US)
