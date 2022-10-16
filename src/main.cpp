#include <cmath>
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    // Create window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_Window *window = SDL_CreateWindow("Terrapainter", 100, 100, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

    // Create OpenGL context
    SDL_GLContext context = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Set up IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiStyle &style = ImGui::GetStyle();
    ImGuiIO &io = ImGui::GetIO();

    // Initialize GLAD
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        error("Failed to initialize GLAD");
    }

    // Set viewport
    glViewport(0, 0, 800, 600);

    bool running = true;
    bool show_demo_window = true;

    Painter painter(800, 600);

    // Run the event loop
    SDL_Event windowEvent;
    while (running)
    {
        // handle events
        while (SDL_PollEvent(&windowEvent))
        {
            ImGui_ImplSDL2_ProcessEvent(&windowEvent);
            painter.process_event(windowEvent);
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
        if (show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // swap buffers
        SDL_GL_SwapWindow(window);
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
