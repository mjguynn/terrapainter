#include "sky.h"
#include "../helpers.h"
#include "../shadermgr.h"
#include <SDL.h>

Sky::Sky(std::string skyboxName) 
	: Entity(vec3::zero(), vec3::zero(), vec3::splat(1))
{
	RGBAImage back("textures/" + skyboxName + "/back.jpg");
	RGBAImage bottom("textures/" + skyboxName + "/bottom.jpg");
	RGBAImage front("textures/" + skyboxName + "/front.jpg");
	RGBAImage left("textures/" + skyboxName + "/left.jpg");
	RGBAImage right("textures/" + skyboxName + "/right.jpg");
	RGBAImage top("textures/" + skyboxName + "/top.jpg");
	auto [w, h] = back.size();
	glGenTextures(1, &mTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mTexture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, w, h);
    glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, right.data());
	glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, left.data());
	glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, bottom.data());
	glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, top.data());
	glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, front.data());
	glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, back.data());

	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);
	glGenBuffers(1, &mVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	static float CUBE_VERTS[] = {
        // Taken from LearnOpenGL
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
	};
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

    mSkyboxProgram = g_shaderMgr.graphics("sky");

	float length = 8192 * 3;

	std::vector<float> positions{
		+length, +length, 3000.0f,
		+length, -length, 3000.0f,
		-length, -length, 3000.0f,
		-length, +length, 3000.0f,
	};

	std::vector<float> uvs{
		1.f, 1.f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f
	};

	std::vector<unsigned int> ind{
		0, 1, 3,
		1, 2, 3
	};

	Geometry tGeo = Geometry();
	tGeo.setIndex(ind);
	tGeo.setAttr("position", Attribute(&positions, 3));
	tGeo.setAttr("texcoord", Attribute(&uvs, 2));

	mClouds = std::make_unique<Mesh>( Material("clouds"), std::move(tGeo));
}
Sky::~Sky() noexcept {
	assert(mTexture);
	glDeleteTextures(1, &mTexture);
	assert(mVAO);
	glDeleteVertexArrays(1, &mVAO);
	assert(mVBO);
	glDeleteVertexArrays(1, &mVBO);
}
void Sky::draw(const RenderCtx& c) const {

	mat4 translate = mat4::translate_hmg(c.viewPos);

    glBindVertexArray(mVAO);
    glUseProgram(mSkyboxProgram->id());
    mat4 xform = translate * mat4::diag(8192, 8192, 8192, 1);
    glUniformMatrix4fv(0, 1, GL_TRUE, c.viewProj.data());
    glUniformMatrix4fv(1, 1, GL_TRUE, xform.data());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTexture);
    glUniform1i(2, 0);
    glUniform3fv(3, 1, c.viewPos.data());
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

	glUseProgram(mClouds->mat().id());
	mClouds->mat().setMat4Float("u_worldToProjection", c.viewProj);
	mClouds->mat().setMat4Float("u_modelToWorld", translate);
	float time = double(SDL_GetTicks64()) / 1000.0;
	mClouds->mat().setFloat("iTime", time);
	mClouds->draw();
}