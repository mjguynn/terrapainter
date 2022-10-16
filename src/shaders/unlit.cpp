#include "unlit.h"
UnlitShader::UnlitShader() 
	: mProgram(g_shaderMgr.program("unlit")), 
	mTransformLocation(0) // Temp value 
{
	glUseProgram(mProgram);
	glUniform1i(glGetUniformLocation(mProgram, "u_texture"), 0);
	mTransformLocation = glGetUniformLocation(mProgram, "u_transform");
}

void UnlitShader::use(const mat4& transform, GLuint texture) {
	glUseProgram(mProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniformMatrix4fv(
		mTransformLocation, 
		1, 
		GL_FALSE, 
		reinterpret_cast<const GLfloat*>(&transform)
	);
}