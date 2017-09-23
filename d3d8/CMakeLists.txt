set(throtator_target d3d8)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libci")

set(throtator_dll_def my_d3d8.def)

include(../common/ConfigureTHRotator.cmake)

set(imgui_d3d8_dir "./")

target_include_directories(d3d8
	PRIVATE "d3d8/include/"
	PRIVATE "../common/"
	PRIVATE ${BOOST_INCLUDE_DIR}
	PRIVATE "../nlohmann-json/src/"
	PRIVATE "../fmtlib/"
	PRIVATE "../imgui/"
	PRIVATE ${imgui_d3d8_dir})

set(imgui_d3d8_source "${imgui_d3d8_dir}imgui_impl_dx8.cpp")
set(imgui_d3d8_header "${imgui_d3d8_dir}imgui_impl_dx8.h")

target_sources(d3d8
	PRIVATE ${imgui_d3d8_source}
	PRIVATE ${imgui_d3d8_header})

source_group("Source Files\\imgui" FILES ${imgui_d3d8_source})
source_group("Header Files\\imgui" FILES ${imgui_d3d8_header})

target_compile_definitions(d3d8
	PRIVATE TOUHOU_ON_D3D8=1)
