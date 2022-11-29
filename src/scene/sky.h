#pragma once
#include <glad/gl.h>

#include "terrapainter/scene/entity.h"

class Sky : public Entity {
	GLuint mTexture;
	GLuint mVAO;
	GLuint mVBO;
public:
	Sky(std::string skyboxName);
	~Sky() noexcept override;

	void draw(ivec2 viewportSize, const mat4& viewProj, vec3 viewPos, vec4 cullPlane) const override;
};