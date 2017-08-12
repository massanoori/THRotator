set(throtator_target d3d9)

link_directories("d3dx/lib_x86/")

set(throtator_dll_def my_d3d9.def)

include(../common/ConfigureTHRotator.cmake)

target_include_directories(d3d9
	PRIVATE "d3dx/include/"
	PRIVATE "../common/"
	PRIVATE ${BOOST_INCLUDE_DIR}
	PRIVATE "../nlohmann-json/src/"
	PRIVATE "../fmtlib/")
