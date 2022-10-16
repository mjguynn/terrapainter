#pragma once
#include <string>
#include <unordered_map>
#include "glad/gl.h"

// Manages loading and compilation of shader programs.
class ShaderManager {
public:
	ShaderManager();
	~ShaderManager();

	GLuint program(std::string shaderName);

private:
	// Map from shader program name to shader program
	// All shader names are in lowercase
	std::unordered_map<std::string, GLuint> mPrograms;
};

extern ShaderManager g_shaderMgr;