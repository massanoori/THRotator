﻿file(GLOB source_files "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.h")

set(localization_target localization_${language})

# Don't generate manifest since muirct emits errors
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")

set(localization_resource_header ${CMAKE_CURRENT_SOURCE_DIR}/resource.h)
set(localization_resource ${CMAKE_CURRENT_SOURCE_DIR}/THRotator_${language}.rc)

add_library(${localization_target} SHARED ${source_files} ${localization_resource} ${localization_resource_header})

add_dependencies(${localization_target} ${throtator_dll_targets})

foreach (throtator_dll_target ${throtator_dll_targets})
	set(throtator_dll_target_file $<TARGET_FILE:${throtator_dll_target}>)
	set(throtator_dll_target_name $<TARGET_FILE_NAME:${throtator_dll_target}>)
	set(throtator_dll_target_dir $<TARGET_FILE_DIR:${throtator_dll_target}>)
	set(main_mui_dir ${throtator_dll_target_dir}/${language})
	set(main_mui ${main_mui_dir}/${throtator_dll_target_name}.mui)

	set(loc_target_file $<TARGET_FILE:${localization_target}>)

	# multilingual user interface (MUI) resource creation
	add_custom_command(TARGET ${localization_target}
		POST_BUILD
		COMMAND if not exist ${main_mui_dir} mkdir \"${main_mui_dir}\"
		COMMAND muirct -q ${CMAKE_CURRENT_LIST_DIR}/DoReverseMuiLoc.rcconfig -x ${language_id} -g 0x0409 ${loc_target_file} ${loc_target_file}.discarded ${main_mui}
		COMMAND muirct -c ${throtator_dll_target_file} -e ${main_mui})

	install(FILES ${main_mui} DESTINATION ${throtator_dll_target}/${language})
endforeach ()
