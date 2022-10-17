#include <array>
#include <memory>
#include <limits>
#include <iostream>
#include <fstream>
#include <optional>
#include <utility>
#include <vector>
#include "terrapainter/util.h"
#include "shadermgr.h"

using namespace std::literals::string_literals;

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
ShaderManager::Program::Program() 
	: mProgram(glCreateProgram()), 
	vertex(std::nullopt), 
	fragment(std::nullopt), 
	compute(std::nullopt) 
{
	// Nothing else, for now
}
ShaderManager::Program::Program(Program&& moved) noexcept
	: mProgram(moved.mProgram),
	vertex(std::move(moved.vertex)),
	fragment(std::move(moved.fragment)),
	compute(std::move(moved.compute))
{
	moved.mProgram = 0;
}
ShaderManager::Program::~Program() { 
	if (mProgram) {
		glDeleteProgram(mProgram);
	}
}
GLuint ShaderManager::Program::id() const { 
	return mProgram; 
}
bool ShaderManager::Program::rebuild() {
	std::vector<GLuint> shaders;
	bool stageCompilationFailed = false;

	auto try_compile = [&](GLenum mode, const std::optional<std::string>& path) {
		if (path.has_value()) {
			auto maybe = load_shader_from_file(mode, path.value());
			if (maybe.has_value()) {
				shaders.push_back(maybe.value());
			} else {
				stageCompilationFailed = true;
			}
		}
	};

	try_compile(GL_VERTEX_SHADER, vertex);
	try_compile(GL_FRAGMENT_SHADER, fragment);
	try_compile(GL_COMPUTE_SHADER, compute);

	if (stageCompilationFailed) {
		fprintf(stderr, "[error] shader linkage failed because one or more shaders failed to compile");
		return false;
	}

	for (auto shader : shaders) {
		glAttachShader(mProgram, shader);
	}

	glLinkProgram(mProgram);

	for (auto shader : shaders) {
		glDetachShader(mProgram, shader);
		glDeleteShader(shader);
	}

	int success;
	glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
	if (!success) {
		char info[1024];
		glGetProgramInfoLog(mProgram, sizeof(info), NULL, info);
		fprintf(stderr, "[error] shader linkage failed\n[details]\n");
		fprintf(stderr, "%s\n", info);
		return false;
	} else {
		return true;
	}
}

ShaderManager::ShaderManager() : mPrograms() {
	// Nothing here, for now...
}

template<typename T>
GLuint ShaderManager::find_or_create(std::string shaderName, T callback) {
	auto location = mPrograms.find(shaderName);
	if (location != mPrograms.end()) {
		return location->second.id();
	}
	Program p;
	callback(&p);
	GLuint id = p.id();
	mPrograms.insert({ shaderName, std::move(p) });
	return id;
}
GLuint ShaderManager::graphics(std::string shaderName) {
	return find_or_create(shaderName, [shaderName](Program* p) {
		p->vertex = "shaders/"s + shaderName + ".vert";
		p->fragment = "shaders/"s + shaderName + ".frag";
		p->rebuild();
	});
}
GLuint ShaderManager::compute(std::string shaderName) {
	return find_or_create(shaderName, [shaderName](Program* p) {
		p->compute = "shaders/"s + shaderName + ".comp";
		p->rebuild();
	});
}
void ShaderManager::refresh() {
	fprintf(stderr, "[info] refreshing all shaders...");
	for (auto& [_, program] : mPrograms) {
		program.rebuild();
	}
	fprintf(stderr, " done\n");
}
ShaderManager::~ShaderManager() {
	// There used to be stuff in here, but now it's in
	// ShaderManager::Program::~Program()
}
ShaderManager g_shaderMgr;