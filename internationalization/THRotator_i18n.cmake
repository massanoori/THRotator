file(GLOB source_files "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.h")

set(localization_target localization_${language})

# Don't generate manifest since muirct emits errors
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")

add_library(${localization_target} SHARED ${source_files} ${localization_resource} ${localization_resource_header})

add_dependencies(${localization_target} d3d8 d3d9)

set(d3d8_target_file $<TARGET_FILE:d3d8>)
set(d3d8_target_name $<TARGET_FILE_NAME:d3d8>)
set(d3d8_target_dir $<TARGET_FILE_DIR:d3d8>)
set(d3d8_mui_dir ${d3d8_target_dir}/${language})
set(d3d8_mui ${d3d8_mui_dir}/${d3d8_target_name}.mui)

set(d3d9_target_file $<TARGET_FILE:d3d9>)
set(d3d9_target_name $<TARGET_FILE_NAME:d3d9>)
set(d3d9_target_dir $<TARGET_FILE_DIR:d3d9>)
set(d3d9_mui_dir ${d3d9_target_dir}/${language})
set(d3d9_mui ${d3d9_mui_dir}/${d3d9_target_name}.mui)

set(loc_target_file $<TARGET_FILE:${localization_target}>)

# multilingual user interface (MUI) resource creation
add_custom_command(TARGET ${localization_target}
	POST_BUILD
	COMMAND if not exist ${d3d8_mui_dir} mkdir \"${d3d8_mui_dir}\"
	COMMAND if not exist ${d3d9_mui_dir} mkdir \"${d3d9_mui_dir}\"
	COMMAND muirct -q ${CMAKE_CURRENT_LIST_DIR}/DoReverseMuiLoc.rcconfig -x ${language_id} -g 0x0409 ${loc_target_file} ${loc_target_file}.discarded ${d3d8_mui}
	COMMAND muirct -q ${CMAKE_CURRENT_LIST_DIR}/DoReverseMuiLoc.rcconfig -x ${language_id} -g 0x0409 ${loc_target_file} ${loc_target_file}.discarded ${d3d9_mui}
	COMMAND muirct -c ${d3d8_target_file} -e ${d3d8_mui}
	COMMAND muirct -c ${d3d9_target_file} -e ${d3d9_mui})
