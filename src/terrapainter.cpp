#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <nfd.hpp>

#include "learnopengl/mesh.h"
#include "canvas/canvas.h"
#include "terrapainter/camera.h"
#include "shadermgr.h"
#include "controllers.h"
#include "terrapainter.h"


static bool should_quit(SDL_Window* main_window, SDL_Event& windowEvent) {
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


// TODO TEMP TEMP TEMP
class HeightmapShader {
    GLuint mProgram;
    GLuint mWorldToProjectionLocation;
    GLuint mModelToWorldLocation;
    GLuint mLightDirLocation;
    GLuint mViewPosLocation;
public:
    HeightmapShader() : mProgram(g_shaderMgr.graphics("heightmap")) {
        mWorldToProjectionLocation = glGetUniformLocation(mProgram, "u_worldToProjection");
        mModelToWorldLocation = glGetUniformLocation(mProgram, "u_modelToWorld");
        mLightDirLocation = glGetUniformLocation(mProgram, "LightDir");
        mViewPosLocation = glGetUniformLocation(mProgram, "viewPos");
    }
    void use(const mat4& worldToProjection, const mat4& modelToWorld, vec3 lightDir, vec3 viewPos) {
        glUseProgram(mProgram);
        glUniformMatrix4fv(mWorldToProjectionLocation, 1, GL_TRUE, worldToProjection.data());
        glUniformMatrix4fv(mModelToWorldLocation, 1, GL_TRUE, modelToWorld.data());
        glUniform3f(mLightDirLocation, lightDir.x, lightDir.y, lightDir.z);
        glUniform3f(mViewPosLocation, viewPos.x, viewPos.y, viewPos.z);
    }
};
static void draw_world(Camera* camera, HeightmapShader& shader, Mesh* map) {
    mat4 viewProj = camera->projection() * camera->world_transform().inverse();
    mat4 model = mat4::ident();
    vec3 lightDir = { 0.0f, 0.0f, -5.0f };
    shader.use(viewProj, model, lightDir, camera->position());
    map->DrawStrips();
}
enum ModalState {
    MODE_CANVAS,
    MODE_WORLD,
    MODE_STOP,
};

static Camera* add_camera(World& world) {
    auto entity = std::make_unique<Camera>(
        vec3{ 0.0f, 0.0f, 400.0f }, // position
        vec3{ 0, -M_PI / 2, M_PI / 2 }, // rotation
        float(M_PI) / 2, // horizontal FOV -- 90 degrees
        ivec2{ 800, 600 }, // screen dimensions: TODO TODO TODO fix this
        vec2{ 0.1f, 100000.0f } // nearZ, farZ
    );
    Camera* camera = entity.get();
    world.add_child(std::move(entity));
    return camera;
}

static void debug_camera(Camera* camera, bool* showDebugCamera = nullptr) {
    if (ImGui::Begin("Camera Controls", showDebugCamera)) {

        vec3 position = camera->position();
        ImGui::DragFloat3("Position", position.data());
        camera->set_position(position);

        vec3 angles = camera->angles() * (180 / M_PI);
        ImGui::DragFloat3("Angles", angles.data());
        camera->set_angles(angles * (M_PI / 180));

        float fov = camera->fov() * (180 / M_PI);
        ImGui::DragFloat("FoV (horizontal)", &fov, 1.0f, 60.0f, 120.0f, "%.1f°");
        camera->set_fov(fov * (M_PI / 180));

        vec2 range = camera->range();
        ImGui::DragFloat2("NearZ/Farz", range.data());
        camera->set_range(range);
    }
    ImGui::End();
}

#if 0 
Mesh* canvas_to_map(const Canvas& canvas) {
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    auto [width, height] = canvas.dimensions();
    auto pixels = canvas.dump_texture();

    std::vector<Vertex> vertices;
    float zScale = 96.0f / 256.0f, zShift = 16.0f;
    int rez = 1;
    unsigned bytePerPixel = 3;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            unsigned char* pixelOffset = (unsigned char*)pixels.data() + (j + width * i) * bytePerPixel;
            unsigned char z = pixelOffset[0];

            vertices.push_back(
                Vertex{
                    .Position = vec3(-width / 2.0f + width * j / (float)width, -height / 2.0f + height * i / (float)height, (int)z * zScale - zShift)
                }
            );
        }
    }
    fprintf(stderr, "[info] generated %llu vertices \n", vertices.size() / 3);

    // ------------------ Normal (start)-------------------------

    // facedata[i] is the vertex index for face i // 3
    std::vector<unsigned int> facedata;
    // loading each face in
    for (int i = 0; i < height - 1; i++)
    {
        for (int j = 0; j < width - 1; j++)
        {
            facedata.push_back(i * width + j);
            facedata.push_back(i * width + j + 1);
            facedata.push_back((i + 1) * width + j);
            facedata.push_back(i * width + j + 1);
            facedata.push_back((i + 1) * width + j + 1);
            facedata.push_back((i + 1) * width + j);
        }
    }

    // normal[i] is the vec3 normal of vertices[i]
    std::vector<vec3> normaldata;
    for (int i = 0; i < vertices.size(); i++)
    {
        normaldata.push_back(vec3(0));
    }

    for (int i = 0; i < facedata.size(); i += 3)
    {
        vec3 v1 = vertices.at(facedata.at(i)).Position;
        vec3 v2 = vertices.at(facedata.at(i + 1)).Position;
        vec3 v3 = vertices.at(facedata.at(i + 2)).Position;

        vec3 side1 = v2 - v1;
        vec3 side2 = v3 - v1;
        vec3 normal = cross(side1, side2);

        normaldata[facedata.at(i)] += normal;
        normaldata[facedata.at(i + 1)] += normal;
        normaldata[facedata.at(i + 2)] += normal;
    }


    for (int i = 0; i < normaldata.size(); i += 1)
    {
        normaldata[i] = normaldata[i].normalize();
        vertices[i].Normal = normaldata[i];
    }

    std::vector<unsigned> indices;
    for (unsigned i = 0; i < height - 1; i += rez)
    {
        for (unsigned j = 0; j < width; j += rez)
        {
            for (unsigned k = 0; k < 2; k++)
            {
                indices.push_back(j + width * (i + k * rez));
            }
        }
    }
    // ------------------- Normal(End) -----------------

    const int numStrips = (height - 1) / rez;
    const int numTrisPerStrip = (width / rez) * 2 - 2;
    fprintf(stderr, "[info] loaded %llu indices\n", indices.size());
    fprintf(stderr, "[info] created lattice of %i strips with %i triangles each\n", numStrips, numTrisPerStrip);
    fprintf(stderr, "[info] created %i triangles total\n", numStrips * numTrisPerStrip);

    return new Mesh(vertices, indices, numTrisPerStrip, numStrips);
}
#endif

void run_terrapainter(SDL_Window* window) {
    ImGuiIO& io = ImGui::GetIO();

    ModalState state = MODE_CANVAS;
    World world;

    Camera* camera = add_camera(world);
    NoclipController cameraController(camera, 0.01, 50.0);
    bool showDebugCamera = false;

    float deltaTime = 0.0f; // time between current frame and last frame
    float lastFrame = 0.0f;

    Canvas canvas({ 800, 600 });
    Mesh* map = nullptr;
    HeightmapShader heightmap_shader;

    // Run the event loop
    SDL_Event windowEvent;
    while (state != MODE_STOP)
    {
        float currentFrame = static_cast<float>(SDL_GetTicks64()) * 0.001f;
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // handle events
        while (SDL_PollEvent(&windowEvent))
        {
            const Uint8* keys = SDL_GetKeyboardState(nullptr);
            if (windowEvent.type == SDL_KEYDOWN) {
                auto pressed = windowEvent.key.keysym.sym;
                auto ctrl = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
                if (pressed == SDLK_F5) {
                    g_shaderMgr.refresh();
                }
                else if (ctrl && pressed == SDLK_o) {
                    load_canvas(canvas);
                }
                else if (ctrl && pressed == SDLK_s) {
                    save_canvas(canvas);
                }
                else if (ctrl && pressed == SDLK_d && state == MODE_WORLD) {
                    showDebugCamera = !showDebugCamera;
                    SDL_SetRelativeMouseMode((SDL_bool)!showDebugCamera);
                }
                else if (pressed == SDLK_SPACE) {
                    // TODO: Refactor this!
                    if (state == MODE_CANVAS) {
                        // switch to world
                        state = MODE_WORLD;
                        // map = canvas_to_map(canvas);
                        SDL_SetRelativeMouseMode((SDL_bool)!showDebugCamera);
                    }
                    else if (state == MODE_WORLD) {
                        // switch to canvas
                        state = MODE_CANVAS;
                        // delete map;
                        map = nullptr;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                    }
                }
            }
            ImGui_ImplSDL2_ProcessEvent(&windowEvent);

            // if (state == MODE_CANVAS) canvas.process_event(windowEvent, io);
            if (state == MODE_WORLD && !showDebugCamera) cameraController.process_event(windowEvent);

            if (should_quit(window, windowEvent)) {
                state = MODE_STOP;
            }

            // This makes dragging windows feel snappy
            io.MouseDrawCursor = ImGui::IsAnyItemFocused() && ImGui::IsMouseDragging(0);
        }
        if (state == MODE_WORLD) {
            cameraController.process_frame(deltaTime);
        }

        // Render scene
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Draw geometry with depth testing
        glDepthFunc(GL_LESS);
        if (state == MODE_WORLD) draw_world(camera, heightmap_shader, map);

        // Draw UI without depth testing
        glDepthFunc(GL_ALWAYS);
        // if (state == MODE_CANVAS) canvas.draw();

        // Render ImGUI ui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        // if (state == MODE_CANVAS) canvas.draw_ui();
        if (state == MODE_WORLD && showDebugCamera) debug_camera(camera, &showDebugCamera);
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