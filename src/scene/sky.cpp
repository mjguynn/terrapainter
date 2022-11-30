#include "sky.h"
#include "../helpers.h"
#include "../shadermgr.h"
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

    mProgram = g_shaderMgr.graphics("sky");
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
    glBindVertexArray(mVAO);
    glUseProgram(mProgram);
    mat4 xform = mat4::translate_hmg(c.viewPos) * mat4::diag(8192, 8192, 8192, 1);
    glUniformMatrix4fv(0, 1, GL_TRUE, c.viewProj.data());
    glUniformMatrix4fv(1, 1, GL_TRUE, xform.data());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTexture);
    glUniform1i(2, 0);
    glUniform3fv(3, 1, c.viewPos.data());
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}