#include <algorithm>
#include <memory>
#include <limits>
#include <iostream>
#include <fstream>
#include "terrapainter/util.h"
#include "shadermgr.h"

using namespace std::literals::string_literals;

ShaderManager::ShaderManager() : mPrograms() {
}

GLuint load_shader_from_file(GLenum shaderType, std::string path) {
	std::ifstream shaderFile(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
	if (!shaderFile.is_open()) {
		fprintf(stderr, "[error] Couldn't open shader \"%s\"\n", path.c_str());
		return 0;
	}
	const size_t shaderFileSize = shaderFile.tellg();
	if (shaderFileSize > std::numeric_limits<GLint>::max()) {
		fprintf(stderr, "[error] Shader file \"%s\" too large\n", path.c_str());
		return 0;
	}
	auto shaderSource = std::make_unique<char[]>(shaderFileSize);
	shaderFile.seekg(0).read(shaderSource.get(), shaderFileSize);

	auto shader = glCreateShader(shaderType);
	const char* shaderSources[1] = { shaderSource.get() };

	// We checked this already...
	DIAG_PUSHIGNORE_MSVC(4838);
	DIAG_PUSHIGNORE_GCC("narrowing")
	GLint shaderSourceSizes[1] = { shaderFileSize };
	DIAG_POP_GCC();
	DIAG_POP_MSVC();

	glShaderSource(shader, 1, shaderSources, shaderSourceSizes );
	glCompileShader(shader);
	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char info[1024];
		glGetShaderInfoLog(shader, sizeof(info), NULL, info);
		fprintf(stderr, "[error] shader compilation failed: \"%s\"\n[details]\n", path.c_str());
		fprintf(stderr, "%s\n", info);
	}

	return shader;
}
GLuint ShaderManager::program(std::string shaderName) {
	for (auto& c : shaderName) {
		c = std::tolower(c);
	}

	auto location = mPrograms.find(shaderName);
	if (location != mPrograms.end()) {
		return location->second;
	}

	// Load shaders...
	auto vert = load_shader_from_file(GL_VERTEX_SHADER, "shaders/"s + shaderName + ".vert");
	auto frag = load_shader_from_file(GL_FRAGMENT_SHADER, "shaders/"s + shaderName + ".frag");

	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char info[1024];
		glGetProgramInfoLog(program, sizeof(info), NULL, info);
		fprintf(stderr, "[error] shader linkage failed: \"%s\"\n[details]\n", shaderName.c_str());
		fprintf(stderr,"%s\n", info);
	}

	glDeleteShader(frag);
	glDeleteShader(vert);

	mPrograms.insert({ shaderName, program });
	return program;
}
ShaderManager::~ShaderManager() {
	for (auto& [_, program] : mPrograms) {
		glDeleteProgram(program);
	}
}

ShaderManager g_shaderMgr;