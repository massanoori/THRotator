set(throtator_target d3d9)

set(throtator_dll_def my_d3d9.def)

include(../common/ConfigureTHRotator.cmake)

target_include_directories(d3d9
	PRIVATE "../common/"
	PRIVATE ${BOOST_INCLUDE_DIR}
	PRIVATE "../nlohmann-json/include/"
	PRIVATE "../fmtlib/include/"
	PRIVATE "../imgui/")
