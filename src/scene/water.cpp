#include "water.h"
#include "../shadermgr.h"

// sets up an infinite plane at Z=0
static void configure_infinite_plane(GLuint vao, GLuint vbo) {
	// below geometry comes from https://stackoverflow.com/a/12965697
	// why the 1s in the Z coordinate? well, we want to translate the plane down...
	// but we can't actually do that since translation depends on the homogenous coordinate
	// being 1!
	// ...but if z=1, then we can "translate it down" by scaling the z axis alone!
	static float VERTS[] = {
		// POSITION (XYZ)		// NORMAL
		-1.0f,	-1.0f, 0.0f,	0.0f, 0.0f, 1.0f,
		-1.0f,  +1.0f, 0.0f,	0.0f, 0.0f, 1.0f,
		+1.0f,  -1.0f, 0.0f,	0.0f, 0.0f, 1.0f,
		+1.0f,  +1.0f, 0.0f,	0.0f, 0.0f, 1.0f,
	};
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VERTS), VERTS, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (const GLvoid*)(0));
	glEnableVertexAttribArray(0); // POSITION (XYZ)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (const GLvoid*)(3*sizeof(float)));
	glEnableVertexAttribArray(1); // NORMAL
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
Water::Water(float height, float seafloorHeight) 
	: Entity(vec3(0, 0, 0), vec3::splat(0), vec3::splat(1))
{
	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	configure_infinite_plane(mVAO, mVBO);

	mSeafloorHeight = seafloorHeight;
	mHeightmapProgram = g_shaderMgr.graphics("heightmap");
}
Water::~Water() {
	assert(mVAO);
	glDeleteVertexArrays(1, &mVAO);
	assert(mVBO);
	glDeleteBuffers(1, &mVBO);
}
void Water::draw(const mat4& viewProj, vec3 viewPos) const {
	const mat4 modelToWorld = world_transform();
	glBindVertexArray(mVAO);
	{
		glUseProgram(mHeightmapProgram);
		// TODO change this
		vec3 lightDir = { 0.0f, 0.0f, -5.0f };
		const mat4 seafloorXform = modelToWorld
			* mat4::diag(2ull << 16, 2ull << 16, 1, 1)
			* mat4::translate_hmg(vec3(0, 0, mSeafloorHeight));
		glUniformMatrix4fv(0, 1, GL_TRUE, viewProj.data());
		glUniformMatrix4fv(1, 1, GL_TRUE, seafloorXform.data());
		glUniform3f(2, lightDir.x, lightDir.y, lightDir.z);
		glUniform3f(3, viewPos.x, viewPos.y, viewPos.z);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	{
		// TODO
	}
	glBindVertexArray(0);
}