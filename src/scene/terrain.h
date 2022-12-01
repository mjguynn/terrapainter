#pragma once

#include "terrapainter/scene/entity.h"
#include "../mesh.h"
#include "../canvas.h"
#include "../shadermgr.h"
#include "../model.h"


class Terrain : public Entity
{
private:
	// Terrain Material
	Program* mGrassProgram;
	Mesh mHeightmap;
	Model mTree;
	Program* mTreeProgram;
	GLuint mGrassVAO;
	GLuint mGrassVBO;
	GLuint mNumGrassTriangles;
	GLuint mGrassTexture;
	GLuint mSeafloorVAO;
	GLuint mSeafloorVBO;
	float mAlphaTest = 0.25f;
	float mAlphaMultiplier = 1.5f;
	
public:
	Terrain(vec3 position, vec3 angles, vec3 scale);
	~Terrain() noexcept override;

	void generate(const Canvas &source);

	void draw(const RenderCtx& c) const override;
};