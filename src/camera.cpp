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
	// Adapted from https://ogldev.org/www/tutorial12/tutorial12.html
	// However, we use horizontal FOV instead of vertical FOV
	float oo_nearz_sub_farz = 1.0f / (mRange.x - mRange.y);
	float oo_tan_half_fov = 1.0f / std::tanf(mFov / 2);
	float aspect = float(mDims.x) / float(mDims.y);
	mBakedProjection = mat4{
		oo_tan_half_fov, 0, 0, 0,
		0, oo_tan_half_fov * aspect, 0, 0,
		0, 0, (-mRange.x - mRange.y) * oo_nearz_sub_farz, (2 * mRange.x * mRange.y) * oo_nearz_sub_farz,
		0, 0, 1, 0
	};
}