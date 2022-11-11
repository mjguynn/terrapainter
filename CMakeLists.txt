cmake_minimum_required(VERSION 3.21)

project(
	terrapainter 	
	VERSION 0.1		
	DESCRIPTION "yes"		
	LANGUAGES CXX C
)

include(CTest)
include(FetchContent)

# ================== DEPENDENCIES ======================
FetchContent_Declare(
	SDL2
	GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
	GIT_TAG origin/main
	GIT_SHALLOW true
	GIT_PROGRESS true
	FIND_PACKAGE_ARGS CONFIG
)
FetchContent_MakeAvailable(SDL2)

FetchContent_Declare(
	Catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG        v3.0.1 # or a later release
	GIT_SHALLOW true
	GIT_PROGRESS true
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
	nfd
	GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
	GIT_TAG origin/master
	GIT_SHALLOW true
	GIT_PROGRESS true
)
FetchContent_MakeAvailable(nfd)

# ================== TERRAPAINTER SETUP ======================
# Dummy library which exists to "bundle up" all the shared compile options
add_library(terrapainter_shared INTERFACE)
target_compile_features(terrapainter_shared INTERFACE cxx_std_20)
set_target_properties(terrapainter_shared PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
	# Parallel compilation
	target_compile_options(terrapainter_shared INTERFACE "/MP")
	# Enable experimental C++ features so I can get Intellisense with concepts
	target_compile_options(terrapainter_shared INTERFACE "/std:c++latest")
	# Nicely group header files for Visual Studio
	source_group("Headers" REGULAR_EXPRESSION "^.+\.h$")
endif()

# Terrapainter lib contains everything that should be easily testable
# so no OpenGL or SDL2 in there
set(terrapainter_lib_SOURCES 
	"${CMAKE_SOURCE_DIR}/src/entity.cpp"
	"${CMAKE_SOURCE_DIR}/src/camera.cpp"
)
set(terrapainter_lib_HEADERS
	"${CMAKE_SOURCE_DIR}/include/terrapainter/camera.h"
	"${CMAKE_SOURCE_DIR}/include/terrapainter/entity.h"
	"${CMAKE_SOURCE_DIR}/include/terrapainter/math.h"
	"${CMAKE_SOURCE_DIR}/include/terrapainter/pixel.h"
	"${CMAKE_SOURCE_DIR}/include/terrapainter/util.h"
)

add_library(terrapainter_lib STATIC ${terrapainter_lib_SOURCES} ${terrapainter_lib_HEADERS})
target_include_directories(terrapainter_lib PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(terrapainter_lib PRIVATE terrapainter_shared)

# ========================== GLAD ===========================
set(glad_SOURCES
	"${CMAKE_SOURCE_DIR}/extern/glad/src/gl.c"
)
set(glad_HEADERS
	"${CMAKE_SOURCE_DIR}/extern/glad/include/glad/gl.h"
	"${CMAKE_SOURCE_DIR}/extern/glad/include/KHR/khrplatform.h"
)
add_library(glad STATIC ${glad_SOURCES} ${glad_HEADERS})
target_include_directories(glad PUBLIC "${CMAKE_SOURCE_DIR}/extern/glad/include")

# ========================== IMGUI ===========================
set(imgui_SOURCES
	"${CMAKE_SOURCE_DIR}/extern/imgui/src/imgui.cpp"
	"${CMAKE_SOURCE_DIR}/extern/imgui/src/imgui_demo.cpp"
	"${CMAKE_SOURCE_DIR}/extern/imgui/src/imgui_draw.cpp"
	"${CMAKE_SOURCE_DIR}/extern/imgui/src/imgui_tables.cpp"
	"${CMAKE_SOURCE_DIR}/extern/imgui/src/imgui_widgets.cpp"
	"${CMAKE_SOURCE_DIR}/extern/imgui/src/backends/imgui_impl_opengl3.cpp"
	"${CMAKE_SOURCE_DIR}/extern/imgui/src/backends/imgui_impl_sdl.cpp"
)
set(imgui_HEADERS
	"${CMAKE_SOURCE_DIR}/extern/imgui/include/imgui/imconfig.h"
	"${CMAKE_SOURCE_DIR}/extern/imgui/include/imgui/imgui.h"
	"${CMAKE_SOURCE_DIR}/extern/imgui/include/imgui/imgui_internal.h"
	"${CMAKE_SOURCE_DIR}/extern/imgui/include/imgui/imstb_rectpack.h"
	"${CMAKE_SOURCE_DIR}/extern/imgui/include/imgui/imstb_textedit.h"
	"${CMAKE_SOURCE_DIR}/extern/imgui/include/imgui/imstb_truetype.h"
	"${CMAKE_SOURCE_DIR}/extern/imgui/include/imgui/backends/imgui_impl_opengl3.h"
	"${CMAKE_SOURCE_DIR}/extern/imgui/include/imgui/backends/imgui_impl_opengl3_loader.h"
	"${CMAKE_SOURCE_DIR}/extern/imgui/include/imgui/backends/imgui_impl_sdl.h"
)
add_library(imgui STATIC ${imgui_SOURCES} ${imgui_HEADERS})
target_include_directories(imgui PUBLIC "${CMAKE_SOURCE_DIR}/extern/imgui/include")
# That's right, we link against terrapainter_lib!
# The reason for this is that we need to include our math class to support conversions
# to/from ImVec2 and ImVec4
target_link_libraries(imgui PUBLIC SDL2::SDL2 terrapainter_shared terrapainter_lib)

# ======================= STB STUFF ==========================
set(stb_SOURCES 
	"${CMAKE_SOURCE_DIR}/extern/stb/src/stb_image_impl.cpp"
)
set(stb_HEADERS
	"${CMAKE_SOURCE_DIR}/extern/stb/include/stb/stb_image.h"
	"${CMAKE_SOURCE_DIR}/extern/stb/include/stb/stb_image_write.h"
)
add_library(stb STATIC ${stb_SOURCES} ${stb_HEADERS})
target_include_directories(stb PUBLIC "${CMAKE_SOURCE_DIR}/extern/stb/include")
target_link_libraries(stb PUBLIC terrapainter_shared)

# ================== TERRAPAINTER (BIN) ======================
# Terrapainter binary links to terrapainter lib and has everything else
set(terrapainter_SOURCES
	"${CMAKE_SOURCE_DIR}/src/main.cpp"
	"${CMAKE_SOURCE_DIR}/src/terrapainter.cpp"
	"${CMAKE_SOURCE_DIR}/src/world.cpp"
	"${CMAKE_SOURCE_DIR}/src/terrain.cpp"
	"${CMAKE_SOURCE_DIR}/src/canvas/canvas.cpp"
	"${CMAKE_SOURCE_DIR}/src/canvas/paint.cpp"
	"${CMAKE_SOURCE_DIR}/src/controllers.cpp"
	"${CMAKE_SOURCE_DIR}/src/shadermgr.cpp"
	"${CMAKE_SOURCE_DIR}/src/shaders/screenspace_circle.cpp"
)
set(terrapainter_HEADERS
	"${CMAKE_SOURCE_DIR}/src/terrapainter.h"
	"${CMAKE_SOURCE_DIR}/src/world.h"
	"${CMAKE_SOURCE_DIR}/src/terrain.h"
	"${CMAKE_SOURCE_DIR}/src/canvas/canvas.h"
	"${CMAKE_SOURCE_DIR}/src/canvas/tool.h"
	"${CMAKE_SOURCE_DIR}/src/shadermgr.h"
	"${CMAKE_SOURCE_DIR}/src/controllers.h"
	"${CMAKE_SOURCE_DIR}/src/shaders/screenspace_circle.h"
)

add_executable(terrapainter ${terrapainter_SOURCES} ${terrapainter_HEADERS})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT terrapainter)
set_property(TARGET terrapainter PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/app/")
set_property(TARGET terrapainter PROPERTY VS_DPI_AWARE "ON")
set_property(TARGET terrapainter PROPERTY RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/app>)

# From SDL2 docs, this target is only available on certain platforms
if(TARGET SDL2::SDL2main)
	target_link_libraries(terrapainter PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(terrapainter PRIVATE 
	terrapainter_shared
	terrapainter_lib 
	SDL2::SDL2 
	imgui 
	glad
	nfd
	stb
	${CMAKE_DL_LIBS}
)

if (NOT SDL2_FOUND)
	# Ensure the SDL2 dylib is in the same directory as the binary so the binary
	# can link to the dylib at runtime; this uses an official code snippet from
	# https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html?highlight=target_file_dir#genex:TARGET_RUNTIME_DLLS
	# We only do this if SDL was downloaded, if it was already present on the
	# system then its dylibs will be in PATH
	add_custom_command(
		TARGET terrapainter POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:terrapainter>
		COMMAND_EXPAND_LISTS
	)
endif()

# ================== TERRAPAINTER TESTS =======================
if(NOT Catch2_FOUND)
	list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

include(Catch)

set(terrapainter_tests_SOURCES
	"${CMAKE_SOURCE_DIR}/tests/math.cpp"
)

add_executable(terrapainter_tests ${terrapainter_tests_SOURCES})
target_link_libraries(terrapainter_tests PRIVATE Catch2::Catch2WithMain terrapainter_lib terrapainter_shared)
catch_discover_tests(terrapainter_tests)
