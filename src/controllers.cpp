#include "controllers.h"

NoclipController::NoclipController(Entity* entity, float mouseSensitivity, float movementSpeed, float shiftScale)
    : mEntity(entity), 
    mMouseSensitivity(mouseSensitivity), 
    mMovementSpeed(movementSpeed), 
    mShiftScale(shiftScale)
{}
bool NoclipController::process_event(const SDL_Event& event) {
    if (event.type == SDL_MOUSEMOTION) {
        vec3 angles = mEntity->euler_angles();
        const float SENSITIVITY = 0.01f;
        // Why not clamp to [0,PI]?
        // Because if so, at the extrema view and up become colinear,
        // so the cross product fails and we have no way of knowing what's sideways
        angles.x = std::clamp(angles.x - mMouseSensitivity * event.motion.yrel, 0.1f, float(M_PI) - 0.1f);
        angles.y -= mMouseSensitivity * event.motion.xrel;
        mEntity->set_euler_angles(angles);
        return true;
    }
    return false;
}
void NoclipController::process_frame(float deltaTime) {
    static const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float delta = mMovementSpeed * deltaTime;
    if (keys[SDL_SCANCODE_LSHIFT]) delta *= mShiftScale;

    // Get movement vectors, we only care about pitch and yaw
    vec3 angles = mEntity->euler_angles();
    float sP = std::sin(angles.x);
    float cP = std::cos(angles.x);
    float sY = std::sin(angles.y);
    float cY = std::cos(angles.y);
    vec3 forward = vec3 { -sY*sP, cY*sP, -cP};
    vec3 up = vec3(0, 0, 1);
    vec3 right = cross(forward, vec3(0, 0, 1)).normalize();

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
