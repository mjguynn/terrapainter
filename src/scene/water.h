#pragma once

#include <glad/gl.h>
#include "terrapainter/scene/entity.h"

#include "../canvas.h"

class Water : public Entity {
	float mSeafloorHeight;
	float mWaterHeight;

	Canvas* mCanvas;

	GLuint mReflectionTexture;
	GLuint mSeafoamTexture;
	GLuint mNormal1Texture;
	GLuint mNormal2Texture;

	GLuint mHeightmapProgram;
	GLuint mWaterProgram;

	// VAO and VBO for infinite plane
	// The same geometry is used for the seafloor and the reflective plane
	GLuint mVAO;
	GLuint mVBO;
public:
	Water(float waterHeight, float seafloorHeight, GLuint reflectionTexture, Canvas* canvas);
	~Water() noexcept override;

	void draw(const RenderCtx& c) const override;
};