#include <array>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <nfd.hpp>

#include "canvas/canvas.h"
#include "world.h"
#include "shadermgr.h"

static void load_canvas(Canvas& canvas) {
    nfdu8filteritem_t filters[1] = { { "Images", "png,jpg,tga,bmp,psd,gif" } };
    NFD::UniquePathU8 path = nullptr;
    auto res = NFD::OpenDialog(path, filters, 1);
    if (res == NFD_ERROR) {
        fprintf(stderr, "[error] internal error (load dialog)");
    }
    else if (res == NFD_OKAY) {
        ivec2 canvasSize;
        stbi_uc* pixels = stbi_load(path.get(), &canvasSize.x, &canvasSize.y, nullptr, 4);
        if (pixels) {
            canvas.set_canvas(canvasSize, pixels);
        }
        else {
            fprintf(stderr, "[error] STBI error: %s", stbi_failure_reason());
        }
    }
}
static void save_canvas(Canvas& canvas) {
    auto [width, height] = canvas.get_canvas_size();

    fprintf(stderr, "[info] dumping texture...");
    auto pixels = canvas.get_canvas();
    fprintf(stderr, " complete\n");

    nfdu8filteritem_t filters[1] = { { "PNG Images", "png" } };
    NFD::UniquePathU8 path = nullptr;
    auto res = NFD::SaveDialog(
        path,
        filters,
        1,
        nullptr,
        "output.png"
    );

    if (res == NFD_ERROR) {
        fprintf(stderr, "[error] internal error (save dialog)\n");
    }
    else if (res == NFD_OKAY) {
        stbi_write_png(path.get(), width, height, 4, pixels.data(), width * 4);
        fprintf(stderr, "[info] image saved to \"%s\"\n", path.get());
    }
}

enum class AppState : size_t {
    Canvas = 0,
    World = 1,

    // Doesn't correspond to any concrete app
    Shutdown
};

constexpr size_t NUM_STATES = size_t(AppState::Shutdown);

static void process_window_event(const std::array<IApp*, NUM_STATES>& apps, AppState& appState, const SDL_Event& event) {
    // Strictly speaking we should also check the window ID, but we only have one window right now.
    if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
        appState = AppState::Shutdown;
    }
}

static void process_keyboard_event(const std::array<IApp*, NUM_STATES>& apps, AppState& appState, const SDL_Event& event) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    SDL_Keycode pressed = event.key.keysym.sym;
    if (event.type == SDL_KEYDOWN && pressed == SDLK_F5) {
        g_shaderMgr.refresh();
    }
    else if (event.type == SDL_KEYDOWN && pressed == SDLK_SPACE) {
        // Cycle through apps
        apps.at(size_t(appState))->deactivate();
        appState = static_cast<AppState>((size_t(appState) + 1) % NUM_STATES);
        apps.at(size_t(appState))->activate();
    }
    else {
        apps[size_t(appState)]->process_event(event);
    }
}

static void process_mouse_event(const std::array<IApp*, NUM_STATES>& apps, AppState& appState, const SDL_Event& event) {
    apps.at(size_t(appState))->process_event(event);
}

void run_terrapainter(SDL_Window* window) {
    ImGuiIO& io = ImGui::GetIO();

    Canvas canvas({ 800, 600 }); // TODO: Fix canvas size!
    World world({ 800, 600 }, canvas);
    
    std::array<IApp*, NUM_STATES> apps = {
        &canvas, // AppState::Canvas
        &world // AppState::World
    };
    AppState appState = AppState::Canvas;
    apps.at(size_t(appState))->activate();

    float deltaTime = 0.0f; // time between current frame and last frame
    float lastFrame = 0.0f;

    // Run the event loop
    SDL_Event event;
    while (true) {
        float currentFrame = static_cast<float>(SDL_GetTicks64()) * 0.001f;
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ~~~~~~~ The Holy Message Pump ~~~~~~~~
        // Mostly we just dispatch to the current app, but we add a few global inputs
        // (like F5 for shader refresh) and do some filtering to play nice with Imgui
        while (SDL_PollEvent(&event) && appState != AppState::Shutdown) {
            // Always send events to SDL
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
                case SDL_QUIT:
                    appState = AppState::Shutdown;
                    continue;
                case SDL_WINDOWEVENT:
                    process_window_event(apps, appState, event);
                    continue;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    if (!io.WantCaptureKeyboard) {
                        process_keyboard_event(apps, appState, event);
                    }
                    continue;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEMOTION:
                case SDL_MOUSEWHEEL:
                    if (!io.WantCaptureMouse) {
                        process_mouse_event(apps, appState, event);
                    }
                    continue;
            }
        }
        // Something in the update loop might have told us to shutdown...
        if (appState == AppState::Shutdown) break;

        // This makes dragging windows feel snappy
        io.MouseDrawCursor = ImGui::IsAnyItemFocused() && ImGui::IsMouseDragging(0);

        IApp* app = apps.at(size_t(appState));
        app->process_frame(deltaTime);
        app->render();

        // Render ImGUI ui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        app->run_ui();
        ImGui::ShowMetricsWindow();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Forcibly await all instructions to minimize frame latency
        GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 16 * 1004); // 16ms wait
        glDeleteSync(sync);

        // swap buffers
        SDL_GL_SwapWindow(window);
    }
}