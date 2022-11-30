#pragma once
#include <glad/gl.h>

#include "terrapainter/scene/entity.h"

class Sky : public Entity {
	GLuint mTexture;
	GLuint mVAO;
	GLuint mVBO;
	GLuint mProgram;
public:
	Sky(std::string skyboxName);
	~Sky() noexcept override;

	void draw(const RenderCtx& c) const override;
};