#include <cmath>
#include <functional>
#include <span>
#include <vector>
#include "glad/gl.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "terrapainter/shader_s.h"
#include "terrapainter/util.h"
#include "terrapainter/math.h"
#include "painter.h"

bool should_quit(SDL_Window* main_window, SDL_Event& windowEvent) {
    if (windowEvent.type == SDL_QUIT) {
        return true;
    }
    else if (windowEvent.type == SDL_WINDOWEVENT) {
        uint32_t main_window_id = SDL_GetWindowID(main_window);
        SDL_WindowEvent& we = windowEvent.window;
        return we.event == SDL_WINDOWEVENT_CLOSE
            && we.windowID == main_window_id;
    }
    else {
        return false;
    }
}

struct CommandArg {
    const char* sName;
    const char* lName;
    bool takesValue;
    std::function<void(const char*)> callback;
};

bool parse_cmdline(int argc, char* argv[], const std::span<CommandArg>& args) {
    CommandArg* current = nullptr;
    for (int i = 1; i < argc; i++) {
        if (current) {
            current->callback(argv[i]);
            current = nullptr;
            continue;
        }
        bool matched = false;
        for (auto& arg : args) {
            if (!strcmp(arg.sName, argv[i]) || !strcmp(arg.lName, argv[i])) {
                if (!arg.takesValue) arg.callback(nullptr);
                else current = &arg;
                matched = true;
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

int main(int argc, char *argv[])
{
    int windowX = 100;
    int windowY = 100;
    int viewportWidth = 800;
    int viewportHeight = 600;

    std::vector<CommandArg> args = {
        CommandArg { "-w", "--width", true, [&](const char* w) {viewportWidth = std::stoi(w);}},
        CommandArg { "-h", "--height", true, [&](const char* h) {viewportHeight = std::stoi(h);}},
        CommandArg { "-x", "--xpos", true, [&](const char* x) {windowX = std::stoi(x); }},
        CommandArg { "-y", "--ypos", true, [&](const char* y) {windowY = std::stoi(y); }},
    };

    if (!parse_cmdline(argc, argv, args)) {
        std::exit(-1);
    }

    // Initialize SDL
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "1");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        error("Failed to initialize SDL: %s\n", SDL_GetError());
    }

    // Set version
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    // Create window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    auto screenWidth = displayMode.w;
    auto screenHeight = displayMode.h;
    int fakeViewportHeight = viewportHeight;
    if (viewportWidth == screenWidth && viewportHeight == screenHeight) {
        // Clearly, the user is *trying* to make a fullscreen window.
        // Since we're debugging we want the app to be fast to start.
        // But Windows goes "oh fullscreen app clearly it's a Video Game
        // let me put it in Exclusive Fullscreen Mode which makes your
        // screen lock up for like 5 seconds and then 5 seconds more
        // every time you Alt-Tab™ surely that's a good idea"
        // ... to get around this we just fudge the height a bit
        fakeViewportHeight +=1;
    }
    SDL_Window *window = SDL_CreateWindow(
        "Terrapainter", 
        windowX, 
        windowY, 
        viewportWidth, 
        fakeViewportHeight,
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
    ImGui_ImplOpenGL3_Init("#version 430");

    ImGuiStyle &style = ImGui::GetStyle();
    ImGuiIO &io = ImGui::GetIO();

    // Initialize GLAD
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        error("Failed to initialize GLAD");
    }

    // Set viewport
    glViewport(0, 0, viewportWidth, viewportHeight);
    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool running = true;

    Painter painter(viewportWidth, viewportHeight);

    // Run the event loop
    SDL_Event windowEvent;
    while (running)
    {
        // handle events
        while (SDL_PollEvent(&windowEvent))
        {
            if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_F5) {
                g_shaderMgr.refresh();
            }
            ImGui_ImplSDL2_ProcessEvent(&windowEvent);
            painter.process_event(windowEvent, io);
            // This makes dragging windows feel snappy
            io.MouseDrawCursor = ImGui::IsAnyItemFocused() && ImGui::IsMouseDragging(0);
            if (should_quit(window, windowEvent)) {
                running = false;
            }
            else if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                glViewport(0, 0, windowEvent.window.data1, windowEvent.window.data2);
            }
        }

        // Render scene
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        painter.draw();

        // Render ImGUI ui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        painter.draw_ui();
        ImGui::ShowMetricsWindow();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // swap buffers
        SDL_GL_SwapWindow(window);

        // sync on previous commands -- 
        // this hurts FPS a bit but should provide better latency
        glFinish();
    }

    // Shutdown IMGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // Shutdown SDL
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
