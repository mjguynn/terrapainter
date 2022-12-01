#pragma once
#include <glad/gl.h>

#include "terrapainter/scene/entity.h"
#include "../mesh.h"

class Sky : public Entity {
	GLuint mTexture;
	GLuint mVAO;
	GLuint mVBO;
	Program* mSkyboxProgram;
	std::unique_ptr<Mesh> mClouds;
public:
	Sky(std::string skyboxName);
	~Sky() noexcept override;

	void draw(const RenderCtx& c) const override;
};