#include "sky.h"

Sky::Sky(std::string skyboxName) 
	: Entity(vec3::zero(), vec3::zero(), vec3::splat(1))
{
	glGenTextures(1, &mTexture);
	
}
Sky::~Sky() noexcept {
	assert(mTexture);
	glDeleteTextures(1, &mTexture);
	assert(mVAO);
	glDeleteVertexArrays(1, &mVAO);
	assert(mVBO);
	glDeleteVertexArrays(1, &mVBO);
}