cmake_minimum_required(VERSION 3.21)

project(
	cs4621 	
	VERSION 0.1		
	DESCRIPTION "yes"		
	LANGUAGES CXX
)

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
endif ()

find_package(SDL2 CONFIG QUIET)
if (NOT SDL2_FOUND)
	FetchContent_MakeAvailable(SDL2)
endif()

# ================== CS4621 SETUP ======================
set(SOURCE 
	"src/create_context.cpp"
)

add_executable(cs4621 WIN32 ${SOURCE})

# Set C++ version to C++20 (the new way, for CMake 3.8+)
target_compile_features(cs4621 PUBLIC cxx_std_20)
set_target_properties(cs4621 PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(cs4621 PUBLIC include)

# MSVC-specific options
if(MSVC)
	# Parallel compilation
	target_compile_options(cs4621 PRIVATE "/MP")
endif()

# From SDL2 docs, this target is only available on certain platforms
if(TARGET SDL2::SDL2main)
	target_link_libraries(cs4621 PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(cs4621 PRIVATE glad SDL2::SDL2)

# Ensure the SDL2 dylib is in the same directory as the binary so the binary
# can link to the dylib at runtime; this uses an official code snippet from
# https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=target_file_dir#genex:TARGET_RUNTIME_DLLS
add_custom_command(
	TARGET cs4621 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:cs4621> $<TARGET_FILE_DIR:cs4621>
	COMMAND_EXPAND_LISTS
)