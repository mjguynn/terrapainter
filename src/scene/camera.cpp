#include "terrapainter/scene/camera.h"

Camera::Camera(vec3 position, vec3 euler_angles, float fov, ivec2 sensorSize, vec2 range) :
	Entity::Entity(position, euler_angles, vec3::splat(1.0f)),
	mFov(fov),
	mSensorSize(sensorSize),
	mRange(range)
{}
mat4 Camera::projection() const{
	// Adapted from https://ogldev.org/www/tutorial12/tutorial12.html
	// However, we use horizontal FOV instead of vertical FOV
	// Also, using the default projection matrix, we'd be staring
	// straight down at the ground, but our "engine"s orientation
	// standard is that a rotation of (0,0,0) points along +X
	// To account for this we first rotate the camera 90 degrees
	// along the X axis to get a +Y view, then -90 degrees along
	// the Z axis to get a +X view.
	// ...except we *don't* do this, instead we implicitly
	// rotate the WORLD 90 degrees on the Z axis then -90 degrees
	// on the X axis, then stick it on the right of the standard
	// projection matrix and multiply.
	// I've "inlined" this all into the returned matrix.

	float oo_farz_sub_nearz = 1.0f / (mRange.x - mRange.y);
	float oo_tan_half_fov = 1.0f / std::tan(mFov / 2);
	float aspect = float(mSensorSize.x) / float(mSensorSize.y);
	return mat4 {
		0, -oo_tan_half_fov, 0, 0,
		0, 0, aspect*oo_tan_half_fov, 0,
		-(mRange.x + mRange.y)*oo_farz_sub_nearz, 0, 0, 2*mRange.x*mRange.y*oo_farz_sub_nearz,
		1, 0, 0, 0
	};
}