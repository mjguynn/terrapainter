#pragma once

#include <glad/gl.h>
#include "terrapainter/scene/entity.h"
#include "../shadermgr.h"
#include "../canvas.h"

class Water : public Entity {
	Canvas* mCanvas;

	GLuint mReflectionTexture;
	GLuint mSeafoamTexture;
	GLuint mNormal1Texture;
	GLuint mNormal2Texture;

	Program* mWaterProgram;

	// VAO and VBO for infinite plane
	// The same geometry is used for the seafloor and the reflective plane
	GLuint mVAO;
	GLuint mVBO;
public:
	Water(GLuint reflectionTexture, Canvas* canvas);
	~Water() noexcept override;

	void draw(const RenderCtx& c) const override;
};