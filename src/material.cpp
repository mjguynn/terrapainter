#include <vector>
#include <iostream>
#include <algorithm>
#include <stb/stb_image.h>
#include "helpers.h"
#include "material.h"
#include "shadermgr.h"

Material::Material(const std::string& shaderName, const std::span<Texture>& textures) 
    : mProgram(g_shaderMgr.graphics(shaderName)), texs()
{
  for (auto tex : textures)
  {
      GLuint id;
      glGenTextures(1, &id);
      load_tex_helper(id, tex.path);
      texs.emplace_back(tex, id);
  }
}

Material::Material(const std::string& shaderName) 
    : Material(shaderName, std::span<Texture>()) {}

Material::Material(Material&& other) noexcept
    : mProgram(std::move(other.mProgram)),
    texs(std::move(other.texs))
{}

Material::~Material() noexcept {
    for (auto [_, texInfo] : texs) {
        glDeleteTextures(1, &texInfo);
    }
}
// utility uniform functions
// ------------------------------------------------------------------------
void Material::setBool(const std::string &name, bool value) const
{         
    glUniform1i(mProgram->uniforms().at(name), (int)value);
}
// ------------------------------------------------------------------------
void Material::setInt(const std::string &name, int value) const
{ 
    glUniform1i(mProgram->uniforms().at(name), value);
}
// ------------------------------------------------------------------------
void Material::setFloat(const std::string &name, float value) const
{ 
    glUniform1f(mProgram->uniforms().at(name), value);
}
void Material::set3Float(const std::string &name, vec3 value) const
{ 
    glUniform3fv(mProgram->uniforms().at(name), 1, value.data());
}
void Material::set4Float(const std::string &name, vec4 value) const
{ 
    glUniform4fv(mProgram->uniforms().at(name), 1, value.data());
}
void Material::setMat4Float(const std::string &name, const mat4& value) const
{ 
    glUniformMatrix4fv(mProgram->uniforms().at(name), 1, GL_TRUE, value.data());
}

void Material::load_tex_helper(GLuint texture, std::string fileName)
{
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // load image, create texture and generate mipmaps
  int width, height, nrChannels;
  std::string path = fileName;
  uint8_t* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
  if (data) {
    int levels = (int)std::min(std::log2(width), std::log2(height));
    glTexStorage2D(GL_TEXTURE_2D, levels, GL_RGBA8, width, height);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
  }
  else {
    fprintf(stderr, "[error] failed to load texture \"%s\"\n", path.c_str());
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 0, 0);
  }
}