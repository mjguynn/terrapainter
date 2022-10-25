#include "controllers.h"

NoclipController::NoclipController(Entity* entity, float mouseSensitivity, float movementSpeed, float shiftScale)
    : mEntity(entity), 
    mMouseSensitivity(mouseSensitivity), 
    mMovementSpeed(movementSpeed), 
    mShiftScale(shiftScale)
{}
bool NoclipController::process_event(const SDL_Event& event) {
    if (event.type == SDL_MOUSEMOTION) {
        vec3 angles = mEntity->angles();
        // If we exactly clamped to PI/2, when we looked straight up or down
        // our view angle would become colinear, so the cross product would
        // fail and we wouldn't be able to move sideways.
        // I think there are workarounds, but why bother?
        float limit = float(M_PI / 2.0 - 0.0625);
        angles.y = std::clamp(angles.y - mMouseSensitivity * event.motion.yrel, -limit, limit);
        angles.z -= mMouseSensitivity * event.motion.xrel;
        mEntity->set_angles(angles);
        return true;
    }
    return false;
}
void NoclipController::process_frame(float deltaTime) {
    static const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float delta = mMovementSpeed * deltaTime;
    if (keys[SDL_SCANCODE_LSHIFT]) delta *= mShiftScale;

    // Get movement vectors
    // We can think of the entity's "default orientation" as pointing towards (1, 0, 0)
    // so that's what we need to rotate. We avoid using a matrix since its overkill...
    vec3 angles = mEntity->angles();
    float sY = std::sin(angles.y);
    float cY = std::cos(angles.y);
    float sZ = std::sin(angles.z);
    float cZ = std::cos(angles.z);
    vec3 forward = vec3 { cY*cZ, cY*sZ, sY };
    const vec3 up = vec3(0, 0, 1);
    vec3 right = cross(forward, up).normalize();

    vec3 position = mEntity->position();
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
    if (keys[SDL_SCANCODE_SPACE]) {
        position += delta * up;
    }
    if (keys[SDL_SCANCODE_LCTRL]) {
        position -= delta * up;
    }
    mEntity->set_position(position);
}
