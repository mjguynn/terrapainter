#pragma once

#include "terrapainter/entity.h"
#include "terrapainter/mesh.h"
#include "canvas.h"

class Terrain : public Entity {
private:
	GLuint mProgram;
	GLuint mWorldToProjectionLocation;
	GLuint mModelToWorldLocation;
	GLuint mLightDirLocation;
	GLuint mViewPosLocation;
	std::unique_ptr<Mesh> mMesh;
public:
	Terrain(vec3 position, vec3 angles, vec3 scale);
	~Terrain() noexcept override;

	void generate(const Canvas& source);

	void draw(const mat4& viewProj, vec3 viewPos) const override;
};