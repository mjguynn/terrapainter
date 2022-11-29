#pragma once

#include <glad/gl.h>
#include "terrapainter/scene/entity.h"

class Water : public Entity {
	float mSeafloorHeight;
	float mWaterHeight;

	GLuint mReflectionTexture;
	GLuint mHeightmapProgram;
	GLuint mWaterProgram;

	// VAO and VBO for infinite plane
	// The same geometry is used for the seafloor and the reflective plane
	GLuint mVAO;
	GLuint mVBO;
public:
	Water(float waterHeight, float seafloorHeight, GLuint reflectionTexture);
	~Water() noexcept override;

	void draw(ivec2 viewportSize, const mat4& viewProj, vec3 viewPos, vec4 cullPlane) const override;
};