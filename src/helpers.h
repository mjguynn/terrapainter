#pragma once

inline void configure_texture(GLuint texture, GLenum min, GLenum mag, GLenum sWrap, GLenum tWrap, GLenum internalFormat, GLenum abstractFormat) {
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, 0, 0, 0, abstractFormat, GL_UNSIGNED_BYTE, nullptr);
}