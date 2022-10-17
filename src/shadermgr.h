#pragma once
#include <string>
#include <optional>
#include <unordered_map>
#include "glad/gl.h"

// Manages loading and compilation of shader programs.
class ShaderManager {
public:
	ShaderManager();
	~ShaderManager();

	ShaderManager(const ShaderManager&) = delete;
	ShaderManager& operator= (const ShaderManager&) = delete;

	GLuint graphics(std::string shaderName);
	GLuint compute(std::string shaderName);

	void refresh();
private:
	class Program {
		// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCreateProgram.xhtml
		// 0 is an invalid program ID. So we re-use 0 to indicate that the program
		// ownership has been moved.
		GLuint mProgram;
	public:
		Program();
		~Program();
		Program(Program&& moved);

		Program(const Program&) = delete;
		Program& operator= (const Program&) = delete;
		

		// Returns the OpenGL ID of the program.
		// This is constant throughout the lifetime of the program.
		GLuint id() const;

		bool rebuild();

		// Map from shader stage to shader source
		// You must call rebuild() for any changes to be reflected.
		std::optional<std::string> vertex, fragment, compute;
	};

	template<typename T>
	GLuint find_or_create(std::string shaderName, T callback);

	// Map from shader name (case-sensitive) to shader program
	std::unordered_map<std::string, Program> mPrograms;
};

extern ShaderManager g_shaderMgr;