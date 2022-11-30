#include "water.h"
#include "../shadermgr.h"
#include "../helpers.h"
// sets up an "infinite" horizontal plane at Z=0
static void configure_infinite_plane(GLuint vao, GLuint vbo) {
	static float VERTS[] = {
		// POSITION (XYZ)			// NORMAL
		-8192.f, -8192.f, 0.0f,		0.0f, 0.0f, 1.0f,
		-8192.f, +8192.f, 0.0f,		0.0f, 0.0f, 1.0f,
		+8192.f, -8192.f, 0.0f,		0.0f, 0.0f, 1.0f,
		+8192.f, +8192.f, 0.0f,		0.0f, 0.0f, 1.0f,
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
Water::Water(float waterHeight, float seafloorHeight, GLuint reflectionTexture, Canvas* canvas) 
	: Entity(vec3(0, 0, 0), vec3::splat(0), vec3::splat(1))
{
	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	configure_infinite_plane(mVAO, mVBO);
	mReflectionTexture = reflectionTexture;
	glGenTextures(1, &mNormal1Texture);
	load_mipmap_texture(mNormal1Texture, "wave_normal_01.png");
	glGenTextures(1, &mNormal2Texture);
	load_mipmap_texture(mNormal2Texture, "wave_normal_02.png");
	glGenTextures(1, &mSeafoamTexture);
	load_mipmap_texture(mSeafoamTexture, "seafoam.jpg");
	mCanvas = canvas;
	mWaterHeight = waterHeight;
	mSeafloorHeight = seafloorHeight;
	mWaterProgram = g_shaderMgr.graphics("water");
	mHeightmapProgram = g_shaderMgr.graphics("heightmap");
	
}
Water::~Water() {
	assert(mVAO);
	glDeleteVertexArrays(1, &mVAO);
	assert(mVBO);
	glDeleteBuffers(1, &mVBO);
	// don't delete reflection texture, we don't own it.
	// same for the canvas texture
}
void Water::draw(const RenderCtx& c) const {
	const mat4 modelToWorld = world_transform();
	glBindVertexArray(mVAO);
	{
		// TODO: fix this!
		vec3 lightDir = { 0.0f, 0.0f, -5.0f };
		glUseProgram(mHeightmapProgram);
		// TODO change this
		const mat4 seafloorXform = modelToWorld
			* mat4::translate_hmg(vec3(c.viewPos.x, c.viewPos.y, mSeafloorHeight));
		glUniformMatrix4fv(0, 1, GL_TRUE, c.viewProj.data());
		glUniformMatrix4fv(1, 1, GL_TRUE, seafloorXform.data());
		glUniform3fv(2, 1, lightDir.data());
		glUniform3fv(3, 1, c.viewPos.data());
		glUniform4fv(4, 1, c.cullPlane.data());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	if(!c.inWaterPass) {
		vec3 lightDir = { -0.45399049974f, -0.89100652419, 0.43837114679f };
		// no, this isn't type confusion, I want the float cast last to preserve as much
		// precision as possible
		float time = double(SDL_GetTicks64()) / 1000.0;
		// TODO
		glUseProgram(mWaterProgram);
		const mat4 waterXform = modelToWorld
			* mat4::translate_hmg(vec3(c.viewPos.x, c.viewPos.y, mWaterHeight));
		glUniformMatrix4fv(0, 1, GL_TRUE, c.viewProj.data());
		glUniformMatrix4fv(1, 1, GL_TRUE, waterXform.data());
		glUniform1f(2, time);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mReflectionTexture);
		glUniform1i(3, 0);
		glUniform2iv(4, 1, c.viewportSize.data());
		glUniform4fv(5, 1, c.cullPlane.data());
		glActiveTexture(GL_TEXTURE1);
		GLuint canvasTexture = mCanvas->get_canvas_texture();
		// i am so sorry canvas
		// this is all OpenGL's fault for not separating samplers from textures
		// why would they do this, it's not even how the hardware works
		glBindTexture(GL_TEXTURE_2D, canvasTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glUniform1i(6, 1);
		ivec2 canvasSize = mCanvas->get_canvas_size();
		glUniform2iv(7, 1, canvasSize.data());
		glUniform3fv(8, 1, lightDir.data());
		glUniform3fv(9, 1, c.viewPos.data());
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, mNormal1Texture);
		glUniform1i(10, 2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, mNormal2Texture);
		glUniform1i(11, 3);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, mSeafoamTexture);
		glUniform1i(12, 4);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindTexture(GL_TEXTURE_2D, canvasTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	glBindVertexArray(0);
}