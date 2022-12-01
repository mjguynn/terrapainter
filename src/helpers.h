#pragma once

#include <string>
#include <cmath>
#include <glad/gl.h>
#include <stb/stb_image.h>

#include "terrapainter/math.h"

class RGBAImage {
	uint8_t* mData;
	ivec2 mSize;
public:
	RGBAImage(const std::string& filename) {
		int unused;
		mSize = ivec2::zero();
		mData = stbi_load(filename.c_str(), &mSize.x, &mSize.y, &unused, 4);
		if (!mData) {
			fprintf(stderr, "[error] failed to load texture \"%s\"\n", filename.c_str());
		}
	}
	~RGBAImage() {
		if (mData) stbi_image_free(mData);
	}
	ivec2 size() const { return mSize; }
	uint8_t* data() const { return mData; }
};

inline bool load_mipmap_texture(GLuint texture, std::string fileName)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
	RGBAImage im("textures/" + fileName);
    if (im.data()) {
		auto [w, h] = im.size();
		int levels = (int)std::min(std::log2(w), std::log2(h));
		glTexStorage2D(GL_TEXTURE_2D, levels, GL_RGBA8, w, h);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, im.data());
        glGenerateMipmap(GL_TEXTURE_2D);
		return true;
    }
    else {
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 0, 0);
		return false;
    }
}

inline void configure_texture(GLuint texture, GLenum min, GLenum mag, GLenum sWrap, GLenum tWrap, GLenum internalFormat, GLenum abstractFormat, GLenum type = GL_UNSIGNED_BYTE)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, 0, 0, 0, abstractFormat, type, nullptr);
}

inline void configure_quad(GLuint vao, GLuint vbo)
{
	static float QUAD_VERTS[] = {
			// POSITION (XYZ)		// TEXCOORD
			-1.0f,	1.0f,	1.0f,	0.0f,	1.0f,
			-1.0f,	-1.0f,	1.0f,	0.0f,	0.0f,
			1.0f,	1.0f,	1.0f,	1.0f,	1.0f,
			1.0f,	-1.0f,	1.0f,	1.0f,	0.0f
	};
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTS), QUAD_VERTS, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const GLvoid *)(0));
	glEnableVertexAttribArray(0); // POSITION (XY)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const GLvoid *)(3 * sizeof(float)));
	glEnableVertexAttribArray(1); // TEXCOORD
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
