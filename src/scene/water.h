#pragma once

#include <glad/gl.h>
#include "terrapainter/scene/entity.h"

class Water : public Entity {
	float mSeafloorHeight;

	GLuint mHeightmapProgram;

	// VAO and VBO for infinite plane
	// The same geometry is used for the seafloor and the reflective plane
	GLuint mVAO;
	GLuint mVBO;
public:
	Water(float height, float seafloorHeight);
	~Water() noexcept override;

	void draw(const mat4& viewProj, vec3 viewPos) const override;
};