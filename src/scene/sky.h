#pragma once

#include <glad/gl.h>
#include "terrapainter/scene/entity.h"
#include "terrapainter/scene/mesh.h"

class Sky : public Entity {
	Material* sMat;
	std::unique_ptr<Mesh> mMesh;

public:
	Sky();
	~Sky() noexcept override;

	void draw(ivec2 viewportSize, const mat4& viewProj, vec3 viewPos, vec4 cullPlane) const override;
};