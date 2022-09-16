cmake_minimum_required(VERSION 3.14)

project(cs4621 	VERSION 0.1
				DESCRIPTION "yes"
				LANGUAGES CXX)

# ================== DEPENDENCIES ======================
include(FetchContent)
FetchContent_Declare(
	glad
	GIT_REPOSITORY https://github.com/Dav1dde/glad.git
)
FetchContent_Declare(
	SDL2
	GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
	GIT_TAG main
)

find_package(glad QUIET)
if (NOT glad_FOUND)
	FetchContent_MakeAvailable(glad)
endif (NOT glad_FOUND)

find_package(SDL2 QUIET)
if (NOT SDL2_FOUND)
	FetchContent_MakeAvailable(SDL2)
endif(NOT SDL2_FOUND)

# ================== CS4621 SETUP ======================

add_executable(cs4621 create_context.cpp)

# Set C++ version to C++20 (the new way, for CMake 3.8+)
target_compile_features(cs4621 PUBLIC cxx_std_20)
set_target_properties(cs4621 PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(cs4621 PUBLIC include)
target_link_libraries(cs4621 PUBLIC sdl2)