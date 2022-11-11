#pragma once

#include "SDL.h"
#include "terrapainter/entity.h"

class NoclipController {
	float mMouseSensitivity;
	float mMovementSpeed;
	float mShiftScale;
public:
	NoclipController(float mouseSensitivity, float movementSpeed, float shiftScale = 10.0f);
	bool process_event(Entity* entity, const SDL_Event& event);
	void process_frame(Entity* entity, float deltaTime);
};