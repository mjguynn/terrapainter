cmake_minimum_required(VERSION 3.21)

project(
	terrapainter 	
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
FetchContent_MakeAvailable(glad)

FetchContent_Declare(
	SDL2
	GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
	GIT_TAG origin/main
	GIT_SHALLOW true
	GIT_PROGRESS true
	FIND_PACKAGE_ARGS CONFIG
)
FetchContent_MakeAvailable(SDL2)

# ================== TERRAPAINTER SETUP ======================
set(terrapainter_SOURCES
	"src/create_context.cpp"
)

add_executable(terrapainter WIN32 ${terrapainter_SOURCES})

# Set C++ version to C++20 (the new way, for CMake 3.8+)
target_compile_features(terrapainter PUBLIC cxx_std_20)
set_target_properties(terrapainter PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(terrapainter PUBLIC include)

# MSVC-specific options
if(MSVC)
	# Parallel compilation
	target_compile_options(terrapainter PRIVATE "/MP")
	# Don't reset the active project
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT terrapainter)
endif()

# From SDL2 docs, this target is only available on certain platforms
if(TARGET SDL2::SDL2main)
	target_link_libraries(terrapainter PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(terrapainter PRIVATE glad SDL2::SDL2)

if (NOT SDL2_FOUND)
	# Ensure the SDL2 dylib is in the same directory as the binary so the binary
	# can link to the dylib at runtime; this uses an official code snippet from
	# https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=target_file_dir#genex:TARGET_RUNTIME_DLLS
	# We only do this if SDL was downloaded, if it was already present on the
	# system then its dylibs will be in PATH
	add_custom_command(
		TARGET terrapainter POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:terrapainter> $<TARGET_FILE_DIR:terrapainter>
		COMMAND_EXPAND_LISTS
	)
endif()