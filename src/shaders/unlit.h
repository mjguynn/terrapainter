#pragma once
#include "terrapainter/math.h"
#include "../shadermgr.h"

class UnlitShader {
public:
	UnlitShader();
	void use(const mat4& transform, GLuint texture);
private:
	GLuint mProgram;
	GLuint mTransformLocation;
};