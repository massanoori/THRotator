set(throtator_target d3d8)

set(throtator_dll_def my_d3d8.def)

include(../common/ConfigureTHRotator.cmake)

target_include_directories(d3d8
	PRIVATE "d3d8/include/"
	PRIVATE "../common/"
	PRIVATE "../externals/json-hpp/include/"
	PRIVATE "../externals/fmtlib/include/"
	PRIVATE "../externals/imgui/")

target_compile_definitions(d3d8
	PRIVATE TOUHOU_ON_D3D8=1)
