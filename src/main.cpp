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
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "nfd.hpp"
#include "learnopengl/mesh.h"
#include "terrapainter/util.h"
#include "terrapainter/math.h"
#include "terrapainter/camera.h"
#include "canvas.h"
#include "controllers.h"

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

class Config {
    int mWindowX;
    int mWindowY;
    int mViewportWidth;
    int mViewportHeight;
    stbi_uc* mTextureData;
    int mScreenWidth;
    int mScreenHeight;

    // Try to make all of the window visible
    void adjust_coords() {
        mWindowX -= std::max(0, mWindowX + mViewportWidth - mScreenWidth);
        mWindowY -= std::max(0, mWindowY + mViewportHeight - mScreenHeight);
        mWindowX = std::max(0, mWindowX);
        mWindowY = std::max(0, mWindowY);
    }

    bool is_fullscreen() const {
        return mViewportWidth == mScreenWidth && mViewportHeight == mScreenHeight;
    }
public:
    // REQUIRES SDL to be init.
    Config(int defX, int defY, int defW, int defH, int screenW, int screenH, int argc, char* argv[])
        : mWindowX(defX),
        mWindowY(defY),
        mViewportWidth(defW),
        mViewportHeight(defH),
        mTextureData(nullptr),
        mScreenWidth(screenW),
        mScreenHeight(screenH)
    {
        const char* canvasInput = nullptr;
        std::array<CommandArg, 5> args = {
            CommandArg { "-w", "--width", true, [&](const char* w) {mViewportWidth = std::stoi(w); }},
            CommandArg { "-h", "--height", true, [&](const char* h) {mViewportHeight = std::stoi(h); }},
            CommandArg { "-x", "--xpos", true, [&](const char* x) {mWindowX = std::stoi(x); }},
            CommandArg { "-y", "--ypos", true, [&](const char* y) {mWindowY = std::stoi(y); }},
            CommandArg { "-i", "--input", true, [&](const char* ci) {canvasInput = ci; } }
        };

        if (!parse_cmdline(argc, argv, args)) std::exit(-1);
        if (canvasInput) set_from_image(canvasInput);
        adjust_coords();
    }
    ~Config() noexcept {
        if (mTextureData) stbi_image_free(mTextureData);
    }
    void set_from_image(const char* path) {
        if (mTextureData) stbi_image_free(mTextureData);
        if (!path) return;
        mTextureData = stbi_load(path, &mViewportWidth, &mViewportHeight, nullptr, 3);
        if (!mTextureData) {
            fprintf(stderr, "[error] error loading heightmap: \"%s\"\n", path);
            fprintf(stderr, "[details] %s\n", stbi_failure_reason());
        } else {
            fprintf(stderr, "[info] loaded heightmap \"%s\"\n", path);
        }
        adjust_coords();
    }
    void set_position(int x, int y) {
        mWindowX = x;
        mWindowY = y;
    }

    int gl_width() const { return mViewportWidth;}
    int gl_height() const { return mViewportHeight;}
    int window_x() const { return mWindowX; }
    int window_y() const { 
        if (mWindowY == 0 && !is_fullscreen()) {
            // We want a border!
            return mWindowY + 40;
        } else {
            return mWindowY;
        }
    }
    int window_width() const { return mViewportWidth; }
    int window_height() const {
        if (is_fullscreen()) {
            // Clearly, the user is *trying* to make a fullscreen window.
            // Since we're debugging we want the app to be fast to start.
            // But Windows goes "oh fullscreen app clearly it's a Video Game
            // let me put it in Exclusive Fullscreen Mode which makes your
            // screen lock up for like 5 seconds and then 5 seconds more
            // every time you Alt-Tab™ surely that's a good idea"
            // ... to get around this we just fudge the height a bit
            return mViewportHeight + 1;
        }
        else {
            return mViewportHeight;
        }
    }

    // Possibly nullptr
    unsigned char* texture_data() const { return mTextureData;  }
};

void ui_load_canvas(SDL_Window* window, Config& cfg, Canvas& canvas) {
    nfdu8filteritem_t filters[1] = { { "Images", "png,jpg,tga,bmp,psd,gif" } };
    NFD::UniquePathU8 path = nullptr;
    auto res = NFD::OpenDialog(path, filters, 1);
    if (res == NFD_ERROR) {
        fprintf(stderr, "[error] internal error (load dialog)");
    } else if (res == NFD_OKAY) {
        int x, y;
        SDL_GetWindowPosition(window, &x, &y);
        cfg.set_position(x, y);
        cfg.set_from_image(path.get());
        SDL_SetWindowPosition(window, cfg.window_x(), cfg.window_y());
        SDL_SetWindowSize(window, cfg.window_width(), cfg.window_height());
        glViewport(0, 0, cfg.gl_width(), cfg.gl_height());
        canvas.~Canvas();
        new(&canvas) Canvas(cfg.gl_width(), cfg.gl_height(), cfg.texture_data());
    }
}
void ui_save_canvas(Canvas& canvas) {
    auto [width, height] = canvas.dimensions();

    fprintf(stderr, "[info] dumping texture...");
    auto pixels = canvas.dump_texture();
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
    } else if (res == NFD_OKAY) {
        stbi_write_png(path.get(), width, height, 3, pixels.data(), width * sizeof(RGBu8));
        fprintf(stderr, "[info] image saved to \"%s\"\n", path.get());
    }
}

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
                    .Position = vec3(-height / 2.0f + height * i / (float)height, -width / 2.0f + width * j / (float)width, (int)z * zScale - zShift)
                }
            );
        }
    }
    fprintf(stderr,"[info] generated %llu vertices \n",vertices.size() / 3);

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

// TODO TEMP TEMP TEMP
class HeightmapShader {
    GLuint mProgram;
    GLuint mProjectionLocation;
    GLuint mViewLocation;
    GLuint mModelLocation;
    GLuint mLightDirLocation;
    GLuint mViewPosLocation;
public:
    HeightmapShader() : mProgram(g_shaderMgr.graphics("heightmap")) {
        mProjectionLocation = glGetUniformLocation(mProgram, "projection");
        mViewLocation = glGetUniformLocation(mProgram, "view");
        mModelLocation = glGetUniformLocation(mProgram, "model");
        mLightDirLocation = glGetUniformLocation(mProgram, "LightDir");
        mViewPosLocation = glGetUniformLocation(mProgram, "viewPos");
    }
    void use(const mat4& projection, const mat4& view, const mat4& model, vec3 lightDir, vec3 viewPos) {
        glUseProgram(mProgram);
        glUniformMatrix4fv(mProjectionLocation, 1, GL_TRUE, projection.data());
        glUniformMatrix4fv(mViewLocation, 1, GL_TRUE, view.data());
        glUniformMatrix4fv(mModelLocation, 1, GL_TRUE, model.data());
        glUniform3f(mLightDirLocation, lightDir.x, lightDir.y, lightDir.z);
        glUniform3f(mViewPosLocation, viewPos.x, viewPos.y, viewPos.z);
    }
};
void draw_world(const Config& cfg, Camera* camera, HeightmapShader& shader, Mesh* map) {
    
    mat4 projection = camera->projection();
    mat4 view = camera->world_transform().inverse();
    mat4 model = mat4::ident();
    vec3 lightDir = { 0.0f, 0.0f, 5.0f };
    shader.use(projection, view, model, lightDir, camera->position());
    map->DrawStrips();
}
enum ModalState {
    MODE_CANVAS,
    MODE_WORLD,
    MODE_STOP,
};

Camera* add_camera(const Config& cfg, World& world) {
    auto entity = std::make_unique<Camera>(
        vec3{ 0.0f, 0.0f, 400.0f }, // position
        vec3 { M_PI / 2, 0, 0 }, // rotation
        float(M_PI) / 2, // horizontal FOV -- 90 degrees
        ivec2{ cfg.gl_width(), cfg.gl_height() }, // screen dimensions
        vec2{ 0.1f, 100000.0f } // nearZ, farZ
    );
    Camera* camera = entity.get();
    world.add_child(std::move(entity));
    return camera;
}

void ui_debug_camera(Camera* camera, bool* showDebugCamera = nullptr) {
    if (ImGui::Begin("Camera Controls", showDebugCamera)) {
        vec3 position = camera->position();
        ImGui::DragFloat3("Position", position.data());
        camera->set_position(position);
        vec3 angles = camera->euler_angles() * (180 / M_PI);
        ImGui::DragFloat("Pitch", &angles.x, 1);
        ImGui::DragFloat("Yaw", &angles.y, 1);
        ImGui::DragFloat("Roll", &angles.z, 1);
        camera->set_euler_angles(angles * (M_PI / 180));
        float fov = camera->fov() * (180 / M_PI);
        ImGui::DragFloat("FoV (horizontal)", &fov,1.0f,  60.0f, 120.0f, "%.1f°");
        camera->set_fov(fov * (M_PI / 180));
        vec2 range = camera->range();
        ImGui::DragFloat2("NearZ/Farz", range.data());
        camera->set_range(range);
    }
    ImGui::End();
}

int main(int argc, char *argv[])
{
    // Initialize SDL
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "1");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        error("Failed to initialize SDL: %s\n", SDL_GetError());
    }

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    Config cfg(100, 100, 800, 600, displayMode.w, displayMode.h, argc, argv);

    // Set version
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    // Create window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window *window = SDL_CreateWindow(
        "Terrapainter", 
        cfg.window_x(),
        cfg.window_y(),
        cfg.window_width(),
        cfg.window_height(),
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

    // Initialize file dialogs
    if (NFD::Init() != NFD_OKAY) {
        error("Failed to initialize NFD (file dialogs)");
    }

    // "Initialize" STBI
    stbi_set_flip_vertically_on_load(1);
    stbi_flip_vertically_on_write(1);

    // Set viewport
    glViewport(0, 0, cfg.gl_width(), cfg.gl_height());
    // Enable depth test (for 3D)
    glEnable(GL_DEPTH_TEST);
    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ModalState state = MODE_CANVAS;
    World world;

    Camera* camera = add_camera(cfg, world);
    NoclipController cameraController(camera, 0.01, 50.0);
    bool showDebugCamera = false;

    float deltaTime = 0.0f; // time between current frame and last frame
    float lastFrame = 0.0f;

    Canvas canvas(cfg.gl_width(), cfg.gl_height(), cfg.texture_data());
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
                } else if (ctrl && pressed == SDLK_o) {
                    ui_load_canvas(window, cfg, canvas);
                    // screen dimensions may have changed...
                    camera->set_dims(ivec2{ cfg.gl_width(), cfg.gl_height() });
                } else if (ctrl && pressed == SDLK_s) {
                    ui_save_canvas(canvas);
                } else if (ctrl && pressed == SDLK_d && state == MODE_WORLD) {
                    showDebugCamera = !showDebugCamera;
                    SDL_SetRelativeMouseMode((SDL_bool)!showDebugCamera);
                }
                else if (pressed == SDLK_TAB) {
                    // TODO: Refactor this!
                    if (state == MODE_CANVAS) {
                        // switch to world
                        state = MODE_WORLD;
                        map = canvas_to_map(canvas);
                        SDL_SetRelativeMouseMode((SDL_bool)!showDebugCamera);
                    } else if (state == MODE_WORLD) {
                        // switch to canvas
                        state = MODE_CANVAS;
                        delete map;
                        map = nullptr;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                    }
                }
            }
            ImGui_ImplSDL2_ProcessEvent(&windowEvent);

            if (state == MODE_CANVAS) canvas.process_event(windowEvent, io);
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw geometry with depth testing
        glDepthFunc(GL_LESS);
        if (state == MODE_WORLD) draw_world(cfg, camera, heightmap_shader, map);

        // Draw UI without depth testing
        glDepthFunc(GL_ALWAYS);
        if (state == MODE_CANVAS) canvas.draw();

        // Render ImGUI ui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        if (state == MODE_CANVAS) canvas.draw_ui();
        if (state == MODE_WORLD && showDebugCamera) ui_debug_camera(camera, &showDebugCamera);
        ImGui::ShowMetricsWindow();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // swap buffers
        SDL_GL_SwapWindow(window);

        // sync on previous commands -- 
        // this hurts FPS a bit but should provide better latency
        glFinish();
    }

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
