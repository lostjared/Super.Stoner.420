cmake_minimum_required(VERSION 3.10)
project(SuperStoner420)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf REQUIRED)
#find_library(GLESV2_LIBRARY GLESv2)

option(DEFAULT_FULL "Default Fullscreen" OFF)


include_directories(${CMAKE_SOURCE_DIR}/include ${SDL2_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${CMAKE_PREFIX_PATH}/include/SDL2 ${CMAKE_INSTALL_PREFIX}/include/SDL2)
file(GLOB C_FILES "${CMAKE_SOURCE_DIR}/*.c")

if(WIN32)
	configure_file(
		"${CMAKE_SOURCE_DIR}/SuperStoner420.rc.in"
		"${CMAKE_BINARY_DIR}/SuperStoner420.rc"
		@ONLY
	)
	set(WIN32_RESOURCES "${CMAKE_BINARY_DIR}/SuperStoner420.rc")
endif()

add_executable(SuperStoner420 WIN32 ${C_FILES} src/glad.c ${WIN32_RESOURCES})
target_link_libraries(SuperStoner420 ${SDL2_LIBRARIES} SDL2_mixer::SDL2_mixer SDL2_ttf::SDL2_ttf)
if(WIN32)
	target_link_options(SuperStoner420 PRIVATE -mwindows)
else()
	target_link_libraries(SuperStoner420 m)
endif()
if(DEFAULT_FULL)
	target_compile_definitions(SuperStoner420 PRIVATE DEFAULT_FULL)
endif()
if(GLESV2_LIBRARY)
	target_link_libraries(SuperStoner420 ${GLESV2_LIBRARY})
else()
	message(WARNING "GLESv2 library not found, OpenGL ES linker errors may occur")
endif()
add_definitions(-DHAS_MIXER)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "SuperStoner420")
