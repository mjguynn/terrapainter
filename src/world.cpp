#include "world.h"

World::World(ivec2 viewportSize, Canvas& source) 
	: Entity(vec3::zero(), vec3::zero(), vec3::splat(1)), 
	mSource(source),
    mCameraController(0.01, 50.0), // TODO: Should I really be hardcoding constants here?
    mShowCameraControls(false),
    mOldRelativeMouseMode(SDL_FALSE) // this will be stomped by activate anyways
{
    auto terrain = std::make_unique<Terrain>(
        vec3::zero(), vec3::zero(), vec3::splat(1.f)
    );
    mTerrain = terrain.get();
    this->add_child(std::move(terrain));

    auto camera = std::make_unique<Camera>(
        vec3{ 0.0f, 0.0f, 400.0f }, // position
        vec3{ 0, -M_PI / 2, M_PI / 2 }, // rotation
        float(M_PI) / 2, // horizontal FOV -- 90 degrees
        viewportSize,
        vec2{ 0.1f, 100000.0f } // nearZ, farZ
    );
    mActiveCamera = camera.get();
    this->add_child(std::move(camera));
}

World::~World() noexcept {
	// nothing, for now...
}

void World::run_camera_control_ui() {
    if (ImGui::Begin("Camera Controls", &mShowCameraControls)) {
        vec3 position = mActiveCamera->position();
        ImGui::DragFloat3("Position", position.data());
        mActiveCamera->set_position(position);

        vec3 angles = mActiveCamera->angles() * (180 / M_PI);
        ImGui::DragFloat3("Angles", angles.data());
        mActiveCamera->set_angles(angles * (M_PI / 180));

        float fov = mActiveCamera->fov() * (180 / M_PI);
        ImGui::DragFloat("FoV (horizontal)", &fov, 1.0f, 60.0f, 120.0f, "%.1f°");
        mActiveCamera->set_fov(fov * (M_PI / 180));

        vec2 range = mActiveCamera->range();
        ImGui::DragFloat2("NearZ/Farz", range.data());
        mActiveCamera->set_range(range);
    }
    ImGui::End();
}
void World::activate() {
    // Unsure if I should put the inverse of the OpenGL calls in deactivate.
    glDepthFunc(GL_LESS);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    mOldRelativeMouseMode = SDL_GetRelativeMouseMode();
    SDL_SetRelativeMouseMode((SDL_bool)!mShowCameraControls);
    mTerrain->generate(mSource);
}
void World::deactivate() {
    SDL_SetRelativeMouseMode(mOldRelativeMouseMode);
}
void World::process_event(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        SDL_Keycode pressed = event.key.keysym.sym;
        bool ctrl = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
        if (ctrl && pressed == SDLK_d) {
            SDL_SetRelativeMouseMode((SDL_bool)mShowCameraControls);
            mShowCameraControls = !mShowCameraControls;
        } 
        else if (ctrl && pressed == SDLK_o) {
            // HACKY LEAKY ABSTRACTION:
            // Ensure Ctrl-O and Ctrl-S still work in world mode
            mSource.prompt_open();
            mTerrain->generate(mSource);
        }
        else if (ctrl && pressed == SDLK_s) {
            mSource.prompt_save();
        }
    }
    mCameraController.process_event(mActiveCamera, event);
}
void World::process_frame(float deltaTime) {
    mCameraController.process_frame(mActiveCamera, deltaTime);
}
void render_tree(const Entity* root, const mat4& viewProj, vec3 viewPos) {
    root->draw(viewProj, viewPos);
    for (const auto& child : root->children()) {
        render_tree(child.get(), viewProj, viewPos);
    }
}
void World::render() const {
    // Clear color set in activate()
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    const mat4 view = mActiveCamera->world_transform().inverse();
    const mat4 viewProj = mActiveCamera->projection() * view;
    const vec4 viewPosH = view * mActiveCamera->position().hmg();
    const vec3 viewPos(viewPosH.x, viewPosH.y, viewPosH.z);
    render_tree(this, viewProj, viewPos);
}
void World::run_ui() {
    if (mShowCameraControls) run_camera_control_ui();
}