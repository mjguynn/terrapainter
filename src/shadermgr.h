#pragma once
#include <string>
#include <optional>
#include <unordered_map>
#include <glad/gl.h>

using LocMap = std::unordered_map<std::string, GLint>;

class Program {
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCreateProgram.xhtml
	// 0 is an invalid program ID. So we re-use 0 to indicate that the program
	// ownership has been moved.
	GLuint mProgram;
	// Map from shader stage to shader source
	// You must call rebuild() for any changes to be reflected.
	std::optional<std::string> mVertex, mGeometry, mFragment, mCompute;

	LocMap mAttrs;
	LocMap mUniforms;

	friend class ShaderManager;
public:
	Program();
	~Program();
	Program(Program&& moved) noexcept;

	Program(const Program&) = delete;
	Program& operator= (const Program&) = delete;

	// Returns the OpenGL ID of the program.
	// This is constant throughout the lifetime of the program.
	GLuint id() const { return mProgram; }

	const LocMap& attrs() { return mAttrs; }
	const LocMap& uniforms() { return mUniforms; }

	bool rebuild();
};

// Manages loading and compilation of shader programs.
class ShaderManager {
public:
	ShaderManager();
	~ShaderManager();

	ShaderManager(const ShaderManager&) = delete;
	ShaderManager& operator= (const ShaderManager&) = delete;

	Program* graphics(std::string shaderName);
	Program* geometry(std::string shaderName);
	Program* screenspace(std::string shaderName);
	Program* compute(std::string shaderName);

	// Don't do anything sneaky between begin screenspace and end screenspace
	// Don't bind VAOs, don't change shaders, etc...
	void begin_screenspace(GLuint program);
	void end_screenspace();

	void refresh();

private:

	std::unordered_map<std::string, Program*> mPrograms;

	template<typename T>
	Program* find_or_create(std::string shaderName, T callback);

	// Dummy VAO for screenspace shader
	GLuint mScreenspaceVAO;
};

extern ShaderManager g_shaderMgr;