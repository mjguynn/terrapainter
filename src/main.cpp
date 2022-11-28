#include <functional>
#include <span>
#include <string>

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
    // NVIDIA spams the shit out of this, obscuring literally every useful piece of info
    if (type == GL_DEBUG_TYPE_OTHER) return;
    const char* header = (type == GL_DEBUG_TYPE_ERROR) ? "[error]" : "[info]";
    fprintf(stderr, "%s OpenGL: %s\n", header, message);
}

struct CommandArg {
    const char* sName;
    const char* lName;
    const char* help;
    bool takesValue;
    std::function<void(const char*)> callback;
};

bool parse_cmdline(int argc, char* argv[], const std::span<CommandArg>& args) {
    const CommandArg* current = nullptr;
    for (int i = 1; i < argc; i++) {
        if (current) {
            current->callback(argv[i]);
            current = nullptr;
            continue;
        }
        if (!strcmp("--help", argv[i])) {
            fprintf(stderr, "Terrapainter (built on " __DATE__")\n");
            for (const auto& arg : args) {
                fprintf(stderr, "\t%s, %s\t\t%s\n", arg.sName, arg.lName, arg.help);
            }
            std::exit(0);
        }
        bool matched = false;
        for (const auto& arg : args) {
            if (!strcmp(arg.sName, argv[i]) || !strcmp(arg.lName, argv[i])) {
                if (!arg.takesValue) arg.callback(nullptr);
                else current = &arg;
                matched = true;
                break;
            }
        }
        if (!matched) {
            fprintf(stderr, "Unknown argument %s\n", argv[i]);
            return false;
        }
    }
    if (current) {
        fprintf(stderr, "No option given for argument %s\n", argv[argc - 1]);
        return false;
    }
    return true;
}

ivec2 fix_window_size(ivec2 screenSize, ivec2 windowSize) {
    if (windowSize == screenSize) {
        // Clearly, the user is *trying* to make a fullscreen window.
        // Since we're debugging we want the app to be fast to start.
        // But Windows goes "oh fullscreen app clearly it's a Video Game
        // let me put it in Exclusive Fullscreen Mode which makes your
        // screen lock up for like 5 seconds and then 5 seconds more
        // every time you Alt-Tab™ surely that's a good idea"
        // ... to get around this we just fudge the height a bit
        return { windowSize.x, windowSize.y + 1 };
    }
    else {
        return windowSize;
    }
}

int main(int argc, char *argv[])
{
    ivec2 windowSize = { 1024, 768 };
    ivec2 windowPos = { 200, 200 };
    const char* initialInput = nullptr;
    std::array<CommandArg, 5> args = {
        CommandArg { "-w", "--width", "Sets the window width", true, [&](const char* w) {windowSize.x = std::stoi(w); }},
        CommandArg { "-h", "--height", "Sets the window height", true, [&](const char* h) {windowSize.y = std::stoi(h); }},
        CommandArg { "-x", "--xpos", "Sets the initial window X position", true, [&](const char* x) {windowPos.x = std::stoi(x); }},
        CommandArg { "-y", "--ypos", "Sets the initial window Y position", true, [&](const char* y) {windowPos.y = std::stoi(y); }},
        CommandArg { "-i", "--input", "Opens Terrapainter with the specified input", true, [&](const char* i) {initialInput = i; }}
    };
    parse_cmdline(argc, argv, args);

    // Initialize SDL
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "1");
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "0");
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

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    ivec2 screenSize = { displayMode.w, displayMode.h };
    windowSize = fix_window_size(screenSize, windowSize);

    SDL_Window *window = SDL_CreateWindow(
        "Terrapainter", 
        windowPos.x,
        windowPos.y,
        windowSize.x,
        windowSize.y,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
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

    fprintf(stderr, "[info] renderer: %s\n", glGetString(GL_RENDERER));

    terrapainter::run(window);

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
