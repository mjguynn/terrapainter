#pragma once

#include "terrapainter/scene/entity.h"
#include "learnopengl/mesh.h"
#include "../canvas.h"

class Terrain : public Entity
{
private:
	GLuint mProgram;
	GLuint mGrassProgram;
	std::unique_ptr<Mesh> mMesh;
	GLuint mGrassVAO;
	GLuint mGrassVBO;
	GLuint mNumGrassTriangles;
	GLuint mGrassTexture;
	float mAlphaTest = 0.25f;
	float mAlphaMultiplier = 1.5f;

public:
	Terrain(vec3 position, vec3 angles, vec3 scale);
	~Terrain() noexcept override;

	void generate(const Canvas &source);

	void draw(ivec2 viewportSize, const mat4 &viewProj, vec3 viewPos, vec4 cullPlane) const override;
};