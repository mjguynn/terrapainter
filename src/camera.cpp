#include "terrapainter/camera.h"

Camera::Camera(vec3 position, vec3 euler_angles, float fov, ivec2 dims, vec2 range) :
	Entity::Entity(position, euler_angles, vec3::splat(1.0f)),
	mFov(fov),
	mDims(dims),
	mRange(range),
	mBakedProjection()
{
	bake_projection();
}

void Camera::bake_projection(){
}