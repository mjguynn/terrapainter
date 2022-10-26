#pragma once

#include "terrapainter/entity.h"

// A generic camera object.
// 
// Camera movement is decoupled from the camera itself,
// so this class can be a part of an FPS camera, orthographic
// camera, trackball-momentum camera, etc...
class Camera : public Entity {
	// The horizontal FOV, in radians.
	float mFov;
	// The pixel dimensions of the camera sensor.
	ivec2 mDims;
	// The near and clipping cutoffs, respectively.
	vec2 mRange;
public:
	Camera(vec3 position, vec3 euler_angles, float fov, ivec2 dims, vec2 range);

	float fov() const { return mFov; }
	void set_fov(float radians) { mFov = radians; }

	ivec2 dims() const { return mDims; }
	void set_dims(ivec2 dims) { mDims = dims; }

	vec2 range() const { return mRange; }
	void set_range(vec2 range) { mRange = range; }

	// Returns the projection matrix alone (position/scale invariant)
	mat4 projection() const;
};