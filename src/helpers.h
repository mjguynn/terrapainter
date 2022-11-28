#pragma once

inline void configure_texture(GLuint texture, GLenum min, GLenum mag, GLenum sWrap, GLenum tWrap, GLenum internalFormat, GLenum abstractFormat) {
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, 0, 0, 0, abstractFormat, GL_UNSIGNED_BYTE, nullptr);
}

inline void configure_quad(GLuint vao, GLuint vbo) {
	static float QUAD_VERTS[] = {
		// POSITION (XY)		// TEXCOORD
		-1.0f,	-1.0f, 1.0f,	0.0f, 0.0f,
		-1.0f,  +1.0f, 1.0f,	0.0f, 1.0f,
		+1.0f,  -1.0f, 1.0f,	1.0f, 0.0f,
		+1.0f,  +1.0f, 1.0f,	1.0f, 1.0f,
	};
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTS), QUAD_VERTS, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const GLvoid*)(0));
	glEnableVertexAttribArray(0); // POSITION (XY)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const GLvoid*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1); // TEXCOORD
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}