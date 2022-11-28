#include "controllers.h"

NoclipController::NoclipController(float mouseSensitivity, float movementSpeed, float shiftScale)
    : mMouseSensitivity(mouseSensitivity), 
    mMovementSpeed(movementSpeed), 
    mShiftScale(shiftScale)
{}
bool NoclipController::process_event(Entity* entity, const SDL_Event& event) {
    if (event.type == SDL_MOUSEMOTION) {
        vec3 angles = entity->angles();
        float limit = float(M_PI / 2.0);
        angles.y = std::clamp(angles.y - mMouseSensitivity * event.motion.yrel, -limit, limit);
        angles.z -= mMouseSensitivity * event.motion.xrel;
        entity->set_angles(angles);
        return true;
    }
    return false;
}
void NoclipController::process_frame(Entity* entity, float deltaTime) {
    static const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float delta = mMovementSpeed * deltaTime;
    if (keys[SDL_SCANCODE_LSHIFT]) delta *= mShiftScale;

    // Get movement vectors
    // We can think of the entity's "default orientation" as pointing towards (1, 0, 0)
    // so that's what we need to rotate. We avoid using a matrix since its overkill...
    vec3 angles = entity->angles();
    float sY = std::sin(angles.y);
    float cY = std::cos(angles.y);
    float sZ = std::sin(angles.z);
    float cZ = std::cos(angles.z);
    vec3 forward = vec3 { cY*cZ, cY*sZ, sY };

    // We also calculate the right vector directly from angles
    // This permits the user to look straight down or up
    // TODO: This doesn't account for roll right now
    vec3 right = vec3{ sZ, -cZ, 0 };

    vec3 position = entity->position();
    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
        position += delta * forward;
    }
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
        position -= delta * forward;
    }
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
        position += delta * right;
    }
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
        position -= delta * right;
    }
    entity->set_position(position);
}
