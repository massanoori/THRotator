file(GLOB throtator_source_files "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB throtator_other_files "${CMAKE_CURRENT_LIST_DIR}/*.h" "${CMAKE_CURRENT_LIST_DIR}/*.rc")

set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/stdafx.cpp
	PROPERTIES
	COMPILE_FLAGS "/Ycstdafx.h")

link_directories(${BOOST_LIB_DIR})

foreach (source_file ${throtator_source_files})
	if ("${source_file}" MATCHES "${CMAKE_CURRENT_LIST_DIR}/stdafx.cpp")
		continue()
	endif ()
	set_source_files_properties(${source_file}
		PROPERTIES
		COMPILE_FLAGS "/Yustdafx.h")
endforeach ()

# Don't generate manifest since muirct emits errors
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")

add_library(${throtator_target} SHARED ${throtator_source_files} ${throtator_other_files} "${throtator_dll_def}")

set(target_file \$\(OutDir\)$<TARGET_FILE_NAME:${throtator_target}>)

add_custom_command(TARGET ${throtator_target}
	POST_BUILD
	COMMAND muirct -q ${CMAKE_CURRENT_LIST_DIR}/../internationalization/AllNeutral.rcconfig -x 0x0411 -g 0x0409 ${target_file} ${target_file})
