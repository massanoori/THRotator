file(GLOB source_files "*.cpp" "*.h" "*.rc")

add_library(localization_en-US SHARED ${source_files})

# Don't generate manifest since muirct emits errors
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")