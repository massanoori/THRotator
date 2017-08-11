cmake_minimum_required(VERSION 3.5)

if (NOT "${CMAKE_GENERATOR}" MATCHES "Visual Studio .+")
	message(FATAL_ERROR "The only supported compiler is MSVC.")
endif ()

if ("${CMAKE_GENERATOR}" MATCHES "(Win64|ARM)")
	message(FATAL_ERROR "ARM and x64 are not supported.")
endif()

set (CMAKE_CXX_STANDARD 11)

foreach(flag_var
		CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
	if(${flag_var} MATCHES "/MD")
		string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
	endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)

project(THRotator)

option(USE_UNICODE "Set unicode as character set" ON)

if (USE_UNICODE)
	add_definitions(-D_UNICODE -DUNICODE)
endif ()

set(BOOST_INCLUDE_DIR "" CACHE PATH "boost header file directory")
set(BOOST_LIB_DIR "" CACHE PATH "boost library directory")

add_subdirectory(d3d9)
add_subdirectory(d3d8)
add_subdirectory(localization_en-US)

install(DIRECTORY sample-config DESTINATION .)
install(FILES MANUAL.txt DESTINATION .)

