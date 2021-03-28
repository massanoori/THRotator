file(GLOB throtator_source_files "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB throtator_other_files "${CMAKE_CURRENT_LIST_DIR}/*.h" "${CMAKE_CURRENT_LIST_DIR}/*.rc")
file(GLOB imgui_source_files "${CMAKE_CURRENT_LIST_DIR}/../externals/imgui/*.cpp")
file(GLOB imgui_header_files "${CMAKE_CURRENT_LIST_DIR}/../externals/imgui/*.h")

source_group("Source Files\\imgui" FILES ${imgui_source_files})
source_group("Header Files\\imgui" FILES ${imgui_header_files})

set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/stdafx.cpp
	PROPERTIES
	COMPILE_FLAGS "/Ycstdafx.h")

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

set(throtator_files ${throtator_source_files} ${throtator_other_files} ${imgui_source_files} ${imgui_header_files} "${throtator_dll_def}")

check_symbol_exists("_WIN64" "" x64_architecture)
if (x64_architecture)
	file(GLOB export_asm_file "../${throtator_target}/x64/*.asm")
	source_group("Source Files\\x64" FILES ${export_asm_file})
	set(throtator_files ${throtator_files} ${export_asm_file})
endif()

add_library(${throtator_target} SHARED ${throtator_files})

set_property(TARGET ${throtator_target} PROPERTY CXX_STANDARD 17)

set(target_file \"\$\(OutDir\)$<TARGET_FILE_NAME:${throtator_target}>\")

add_dependencies(${throtator_target} fmt)
target_link_libraries(${throtator_target} "$<TARGET_FILE:fmt>")

add_custom_command(TARGET ${throtator_target}
	POST_BUILD
	COMMAND muirct -q ${CMAKE_CURRENT_LIST_DIR}/../internationalization/AllNeutral.rcconfig -x 0x0411 -g 0x0409 ${target_file} ${target_file})

install(TARGETS ${throtator_target} RUNTIME DESTINATION ${throtator_target})
