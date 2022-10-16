#include <algorithm>
#include <memory>
#include <limits>
#include <iostream>
#include <fstream>
#include <optional>
#include <utility>
#include "terrapainter/util.h"
#include "shadermgr.h"

using namespace std::literals::string_literals;

ShaderManager::ShaderManager() : mPrograms() {
}

std::optional<GLuint> load_shader_from_file(GLenum shaderType, std::string path) {
	std::ifstream shaderFile(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
	if (!shaderFile.is_open()) {
		fprintf(stderr, "[error] couldn't open shader \"%s\"\n", path.c_str());
		return std::nullopt;
	}
	const size_t shaderFileSize = shaderFile.tellg();
	if (shaderFileSize > std::numeric_limits<GLint>::max()) {
		fprintf(stderr, "[error] shader file \"%s\" too large\n", path.c_str());
		return std::nullopt;
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
		glDeleteShader(shader);
		return std::nullopt;
	}

	return shader;
}

// We return a pair instead of an optional because we still *want* to reserve a name for this shader, even if we couldn't compile it.
// This lets us pass the invalid shader ID back and start using it. Then, we can fix the shader and hot-reload it, and all is well in the world.
std::pair<bool, GLuint> load_program_from_files(std::string vertexPath, std::string fragmentPath, const GLuint* existing = nullptr) {
	GLuint program = existing ? *existing : glCreateProgram();

	auto maybe_vert = load_shader_from_file(GL_VERTEX_SHADER, vertexPath);
	auto maybe_frag = load_shader_from_file(GL_FRAGMENT_SHADER, fragmentPath);
	if (!maybe_vert || !maybe_frag) {
		fprintf(stderr, "[error] shader linkage failed because one or more shaders failed to compile");
		return { false, program };
	}
	auto vert = maybe_vert.value();
	auto frag = maybe_frag.value();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	glDetachShader(program, vert);
	glDetachShader(program, frag);
	glDeleteShader(vert);
	glDeleteShader(frag);
	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char info[1024];
		glGetProgramInfoLog(program, sizeof(info), NULL, info);
		fprintf(stderr, "[error] shader linkage failed\n[details]\n");
		fprintf(stderr, "%s\n", info);
		return { false, program };
	}

	return {true, program};

}
GLuint ShaderManager::program(std::string shaderName) {
	for (auto& c : shaderName) c = std::tolower(c);

	auto location = mPrograms.find(shaderName);
	if (location != mPrograms.end()) {
		return location->second;
	}

	auto [result, program] = load_program_from_files(
		"shaders/"s + shaderName + ".vert",
		"shaders/"s + shaderName + ".frag"
	);

	mPrograms.insert({ shaderName, program });
	return program;
}
void ShaderManager::refresh() {
	fprintf(stderr, "[info] refreshing all shaders...");
	for (auto& [name, program] : mPrograms) {
		load_program_from_files(
			"shaders/"s + name + ".vert",
			"shaders/"s + name + ".frag",
			&program
		);
	}
	fprintf(stderr, " done\n");
}
ShaderManager::~ShaderManager() {
	for (auto& [_, program] : mPrograms) {
		glDeleteProgram(program);
	}
}

ShaderManager g_shaderMgr;