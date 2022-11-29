#pragma once
#include <string>

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
			// POSITION (XY)		// TEXCOORD
			-1.0f,
			-1.0f,
			1.0f,
			0.0f,
			0.0f,
			-1.0f,
			+1.0f,
			1.0f,
			0.0f,
			1.0f,
			+1.0f,
			-1.0f,
			1.0f,
			1.0f,
			0.0f,
			+1.0f,
			+1.0f,
			1.0f,
			1.0f,
			1.0f,
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

inline void loadTexture(unsigned int &texture, std::string file_name)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char *data = stbi_load(file_name.c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
}