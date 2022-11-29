#pragma once

#include "terrapainter/scene/entity.h"
#include "learnopengl/mesh.h"
#include "../canvas.h"

class Terrain : public Entity {
private:
	GLuint mProgram;
	std::unique_ptr<Mesh> mMesh;
public:
	Terrain(vec3 position, vec3 angles, vec3 scale);
	~Terrain() noexcept override;

	void generate(const Canvas& source);

	void draw(const mat4& viewProj, vec3 viewPos, vec4 cullPlane) const override;
};