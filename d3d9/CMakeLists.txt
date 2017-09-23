set(throtator_target d3d9)

set(throtator_dll_def my_d3d9.def)

include(../common/ConfigureTHRotator.cmake)

set(imgui_d3d9_dir "../imgui/examples/directx9_example/")

target_include_directories(d3d9
	PRIVATE "../common/"
	PRIVATE ${BOOST_INCLUDE_DIR}
	PRIVATE "../nlohmann-json/src/"
	PRIVATE "../fmtlib/"
	PRIVATE "../imgui/"
	PRIVATE ${imgui_d3d9_dir})

set(imgui_d3d9_source "${imgui_d3d9_dir}imgui_impl_dx9.cpp")
set(imgui_d3d9_header "${imgui_d3d9_dir}imgui_impl_dx9.h")

target_sources(d3d9
	PRIVATE ${imgui_d3d9_source}
	PRIVATE ${imgui_d3d9_header})
	
source_group("Source Files\\imgui" FILES ${imgui_d3d9_source})
source_group("Header Files\\imgui" FILES ${imgui_d3d9_header})
