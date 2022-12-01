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
Program::Program() 
	: mProgram(glCreateProgram()), 
	mVertex(std::nullopt), 
	mGeometry(std::nullopt),
	mFragment(std::nullopt), 
	mCompute(std::nullopt),
	mAttrs(),
	mUniforms()
{
	// Nothing else, for now
}
Program::Program(Program&& moved) noexcept
	: mProgram(moved.mProgram),
	mVertex(std::move(moved.mVertex)),
	mGeometry(std::move(moved.mGeometry)),
	mFragment(std::move(moved.mFragment)),
	mCompute(std::move(moved.mCompute)),
	mAttrs(std::move(moved.mAttrs)),
	mUniforms(std::move(moved.mUniforms))
{
	moved.mProgram = 0;
}
Program::~Program() { 
	// TODO: This is UB if its called while program is closing?
	// The program is closing though so idk if its an issue
	if (mProgram) {
		glDeleteProgram(mProgram);
	}
}
bool Program::rebuild() {
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

	try_compile(GL_VERTEX_SHADER, mVertex);
	try_compile(GL_GEOMETRY_SHADER, mGeometry);
	try_compile(GL_FRAGMENT_SHADER, mFragment);
	try_compile(GL_COMPUTE_SHADER, mCompute);

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
	}

	mUniforms.clear();

	// Get all active uniforms and store them in map of uniform variable locations
	int numUniforms;
	glGetProgramiv(mProgram, GL_ACTIVE_UNIFORMS, &numUniforms);

	int maxUniformLength;
	glGetProgramiv(mProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformLength);
	if (maxUniformLength > 0) {
		char* uniformBuffer = (char*)malloc(sizeof(char) * maxUniformLength);
		for (int i = 0; i < numUniforms; i++)
		{
			int length, size;
			GLenum dataType;
			glGetActiveUniform(mProgram, i, maxUniformLength, &length, &size, &dataType, uniformBuffer);
			GLint varLocation = glGetUniformLocation(mProgram, uniformBuffer);
			std::string name = std::string(uniformBuffer, length);
			mUniforms[name] = varLocation;
		}
		free(uniformBuffer);
	}

	// Do the same for attributes
	int numAttribs;
	glGetProgramiv(mProgram, GL_ACTIVE_ATTRIBUTES, &numAttribs);

	int maxAttrLength;
	glGetProgramiv(mProgram, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxAttrLength);
	if (maxAttrLength > 0) {
		char* attrBuffer = (char*)malloc(sizeof(char) * maxAttrLength);
		for (int i = 0; i < numAttribs; i++)
		{
			int length, size;
			GLenum dataType;
			glGetActiveAttrib(mProgram, i, maxAttrLength, &length, &size, &dataType, attrBuffer);
			GLint varLocation = glGetAttribLocation(mProgram, attrBuffer);
			std::string name = std::string(attrBuffer, length);
			mAttrs[name] = varLocation;
		}

		free(attrBuffer);
	}
	
	return true;
	
}

ShaderManager::ShaderManager() : mPrograms(), mScreenspaceVAO(0) {
	// Nothing here, for now...
}

template<typename T>
Program* ShaderManager::find_or_create(std::string shaderName, T callback) {
	auto location = mPrograms.find(shaderName);
	if (location != mPrograms.end()) {
		return location->second;
	}
	// We *intentionally* leak this, since we want cached programs to persist until
	// program termination and we don't particularly care what happens when we exit.
	Program* p = new Program;
	callback(p);
	mPrograms.insert({ shaderName, p });
	return p;
}
Program* ShaderManager::graphics(std::string shaderName) {
	return find_or_create(shaderName, [shaderName](Program* p) {
		p->mVertex = "shaders/"s + shaderName + ".vert";
		p->mFragment = "shaders/"s + shaderName + ".frag";
		p->rebuild();
	});
}
Program* ShaderManager::screenspace(std::string shaderName) {
	return find_or_create(shaderName, [shaderName](Program* p) {
		p->mVertex = "shaders/screenspace.vert";
		p->mFragment = "shaders/"s + shaderName + ".frag";
		p->rebuild();
	});
}
Program* ShaderManager::compute(std::string shaderName) {
	return find_or_create(shaderName, [shaderName](Program* p) {
		p->mCompute = "shaders/"s + shaderName + ".comp";
		p->rebuild();
	});
}
Program* ShaderManager::geometry(std::string shaderName) {
	return find_or_create(shaderName, [shaderName](Program* p) {
		p->mVertex = "shaders/"s + shaderName + ".vert";
		p->mGeometry = "shaders/"s + shaderName + ".geom";
		p->mFragment = "shaders/"s + shaderName + ".frag";
		p->rebuild();
	});
}
void ShaderManager::refresh() {
	fprintf(stderr, "[info] refreshing all shaders...");
	for (auto& [_, program] : mPrograms) {
		program->rebuild();
	}
	fprintf(stderr, " done\n");
}
void ShaderManager::begin_screenspace(GLuint program) {
	if (!mScreenspaceVAO) {
		glGenVertexArrays(1, &mScreenspaceVAO);
	}
	glBindVertexArray(mScreenspaceVAO);
	glUseProgram(program);
}
void ShaderManager::end_screenspace() {
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
	glUseProgram(0);
	glBindVertexArray(0);
}
ShaderManager::~ShaderManager() {
	// There used to be stuff in here, but now it's in
	// ShaderManager::Program::~Program()
}
ShaderManager g_shaderMgr;