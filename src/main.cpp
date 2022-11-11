#include <glad/gl.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <nfd.hpp>

#include "terrapainter.h"

// These let Terrapainter use the dedicated GPU in dual-GPU systems
// such as gaming laptops. Disabled by default because
//  1. Using the iGPU is actually a bit snapper right now, for some reason
//  2. Using the dGPU drains battery life and we don't need the extra perf anyways
#define TP_ENABLE_DGPU 0
#if TP_ENABLE_DGPU
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

void GLAPIENTRY glDebugLogger(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam
) {
    const char* header = (type == GL_DEBUG_TYPE_ERROR) ? "[error]" : "[info]";
    fprintf(stderr, "%s OpenGL: %s\n", header, message);
}

int main(int argc, char *argv[])
{
    // Initialize SDL
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "1");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        error("Failed to initialize SDL: %s\n", SDL_GetError());
    }

    // Set version
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG | SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG);

    // Create window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window *window = SDL_CreateWindow(
        "Terrapainter", 
        200,
        200,
        800,
        600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI
    );

    // Create OpenGL context
    SDL_GLContext context = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    // Set up IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    // imgui_impl_opengl3 supports OpenGL 3.0+, it's just poorly named
    ImGui_ImplOpenGL3_Init("#version 440");

    // Initialize GLAD
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        error("Failed to initialize GLAD");
    }

    // Initialize file dialogs
    if (NFD::Init() != NFD_OKAY) {
        error("Failed to initialize NFD (file dialogs)");
    }

    // "Initialize" STBI
    stbi_set_flip_vertically_on_load(1);
    stbi_flip_vertically_on_write(1);

    // Enable depth test (for 3D)
    glEnable(GL_DEPTH_TEST);
    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Enable debug/error logging
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glDebugLogger, 0);
    // Set viewport
    glViewport(0, 0, 800, 600);

    fprintf(stderr, "[info] renderer: %s\n", glGetString(GL_RENDERER));

    run_terrapainter(window);

    // Shutdown file dialog
    NFD::Quit();

    // Shutdown IMGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // Shutdown SDL
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    fprintf(stderr, "[info] clean shutdown\n");
    return 0;
}
