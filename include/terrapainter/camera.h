#pragma once

#include "terrapainter/entity.h"

// A generic camera object.
// 
// Camera movement is decoupled from the camera itself,
// so this class can be a part of an FPS camera, orthographic
// camera, trackball-momentum camera, etc...
class Camera : Entity {
	// The horizontal FOV, in radians.
	// If this is 0, the camera is operating in orthographic mode.
	float mFov;
	// The pixel dimensions of the camera sensor.
	ivec2 mDims;
	// The near and clipping cutoffs, respectively.
	vec2 mRange;

	// We don't bother with a dirty flag for this,
	// we just force-update it every time we change
	// any of the above parameters.
	mat4 mBakedProjection;

	void bake_projection();
public:
	Camera(vec3 position, vec3 euler_angles, float fov, ivec2 dims, vec2 range);

	float fov() const { return mFov; }
	void set_fov(float radians) { 
		mFov = radians;
		bake_projection();
	}

	ivec2 dims() const { return mDims; }
	void set_dims(ivec2 dims) {
		mDims = dims;
		bake_projection();
	}

	vec2 range() const { return mRange; }
	void set_range(vec2 range) {
		mRange = range;
		bake_projection();
	}

	// Returns the projection matrix alone (position/scale invariant)
	mat4 projection() const { return mBakedProjection; }
};