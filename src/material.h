#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <span>
#include <glad/gl.h>
#include "terrapainter/math.h"
#include "shadermgr.h"

struct Texture {
    std::string name;
    std::string path;
    GLenum min = GL_LINEAR_MIPMAP_LINEAR;
    GLenum mag = GL_LINEAR;
    GLenum sWrap = GL_REPEAT;
    GLenum tWrap = GL_REPEAT;
    GLenum internalFormat = GL_RGBA8;
    GLenum abstractFormat = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
};

class Material {
  private:
    Program* mProgram;
  public:
    std::vector<std::pair<Texture, GLuint>> texs;
    Material(const std::string& shaderName);
    Material(const std::string& shaderName, const std::span<Texture>& texs);
    ~Material() noexcept;

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&& moved) noexcept;

    GLuint id() const { return mProgram->id(); }

    const LocMap& attrs() const { return mProgram->attrs(); }
    const LocMap& uniforms() const { return mProgram->uniforms(); }

    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const;
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const;
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const;
    void set3Float(const std::string &name, vec3 value) const;
    void set4Float(const std::string &name, vec4 value) const;
    void setMat4Float(const std::string &name, const mat4& value) const;
  private:
    void load_tex_helper(GLuint texture, std::string fileName);
};