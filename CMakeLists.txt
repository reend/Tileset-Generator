Project (atlas)
set(ATLAS_GEN_NAME "gen")
CMake_Minimum_Required (VERSION 3.1)

include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2_IMAGE REQUIRED SDL2_image)

add_compile_options(-Wall -Wempty-body -Werror -Wstrict-prototypes -Werror=maybe-uninitialized -Warray-bounds -g -lefence)

#Add the include directories of the (found) libraries.
Include_Directories(
 	${SDL2_INCLUDE_DIRS}
 	${SDL2_IMAGE_INCLUDE_DIRS}
)

file(GLOB SOURCES   src/defs.h src/structs.h src/*.c src/*.h src/*/*.c src/*/*.h)

SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})
list(FILTER SOURCES EXCLUDE REGEX "src/atlasGen.c")

Add_Executable (${CMAKE_PROJECT_NAME} ${SOURCES})
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)

Target_Link_Libraries (
 	${CMAKE_PROJECT_NAME}
 	${SDL2_LIBRARIES}
 	${SDL2_IMAGE_LIBRARIES}
 	m
)

# Gen
Add_Executable (${ATLAS_GEN_NAME} "src/atlasGen.c" "src/cJSON.c")
set_property(TARGET ${ATLAS_GEN_NAME} PROPERTY CXX_STANDARD 20)

Target_Link_Libraries (
 	${ATLAS_GEN_NAME}
 	${SDL2_LIBRARIES}
 	${SDL2_IMAGE_LIBRARIES}
 	m
)
