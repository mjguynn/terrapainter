#include "sky.h"
#include <vector>
#include "../helpers.h"
#include "../canvas.h"

Sky::Sky() 
	: Entity(vec3(0, 0, 0), vec3::splat(0), vec3::splat(1))
{
	sMat = new Material("clouds");

	float length = 8192 * 3;

	std::vector<float> positions {
		+length, +length, 3000.0f,
		+length, -length, 3000.0f,
		-length, -length, 3000.0f,
		-length, +length, 3000.0f,
	 }; 

	std::vector<float> uvs {
		1.f, 1.f,
		1.0f, 0.0f, 
		0.0f, 0.0f, 
		0.0f, 1.0f
	}; 

	std::vector<unsigned int> ind {
		0, 1, 3,
		1, 2, 3
	};

	Attribute* aPos = new Attribute(&positions, 3);
	Attribute* aUv = new Attribute(&uvs, 2);
	Geometry tGeo = Geometry();
	tGeo.setIndex(ind);
	tGeo.setAttr("position", aPos);
	tGeo.setAttr("texcoord", aUv);

	mMesh = std::make_unique<Mesh>(tGeo, *this->sMat);
}
Sky::~Sky() {
	free(sMat);
}
void Sky::draw(ivec2 viewportSize, const mat4& viewProj, vec3 viewPos, vec4 cullPlane) const {
	const mat4 modelToWorld = world_transform();

	glUseProgram(sMat->progID);
	mMesh->mat.setMat4Float("u_worldToProjection", viewProj);
  mMesh->mat.setMat4Float("u_modelToWorld", modelToWorld);

	float time = double(SDL_GetTicks64()) / 1000.0;
	mMesh->mat.setFloat("iTime", time);

	mMesh->Draw();
}