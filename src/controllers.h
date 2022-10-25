#pragma once

#include "SDL.h"
#include "terrapainter/entity.h"

// Note: This should not outlive the entity it's created on!
class NoclipController {
	Entity* mEntity;
	float mMouseSensitivity;
	float mMovementSpeed;
	float mShiftScale;
public:
	NoclipController(Entity* entity, float mouseSensitivity, float movementSpeed, float shiftScale = 10.0f);
	bool process_event(const SDL_Event& event);
	void process_frame(float deltaTime);
};