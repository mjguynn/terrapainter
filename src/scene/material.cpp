#include "terrapainter/scene/material.h"

#include "vector"
#include "stb/stb_image.h"
#include <iostream>
#include "../shadermgr.h"
#include <algorithm>
#include "../helpers.h"

Material::Material(std::string shaderName, std::vector<Texture> textures) {
  progID = g_shaderMgr.graphics(shaderName);
  g_shaderMgr.mPrograms[shaderName].rebuild();
  attrLocs = g_shaderMgr.mPrograms[shaderName].attrLocs;
  uniformLocs = g_shaderMgr.mPrograms[shaderName].uniformLocs;

  texs = textures;
  texIds = std::vector<unsigned int>(texs.size());
  std::transform(texs.cbegin(), texs.cend(), std::back_inserter(texNames),
                [](Texture t) { return t.name; });
  setupTexs();
}

Material::Material(std::string shaderName) {
  progID = g_shaderMgr.graphics(shaderName);
  g_shaderMgr.mPrograms[shaderName].rebuild();
  attrLocs = g_shaderMgr.mPrograms[shaderName].attrLocs;
  uniformLocs = g_shaderMgr.mPrograms[shaderName].uniformLocs;
}

// utility uniform functions
// ------------------------------------------------------------------------
void Material::setBool(const std::string &name, bool value) const
{         
    glUniform1i(glGetUniformLocation(progID, name.c_str()), (int)value); 
}
// ------------------------------------------------------------------------
void Material::setInt(const std::string &name, int value) const
{ 
    glUniform1i(glGetUniformLocation(progID, name.c_str()), value); 
}
// ------------------------------------------------------------------------
void Material::setFloat(const std::string &name, float value) const
{ 
    glUniform1f(glGetUniformLocation(progID, name.c_str()), value); 
}
void Material::set3Float(const std::string &name, vec3 value) const
{ 
    glUniform3fv(glGetUniformLocation(progID, name.c_str()), 1, value.data()); 
}
void Material::set4Float(const std::string &name, vec4 value) const
{ 
    glUniform4fv(glGetUniformLocation(progID, name.c_str()), 1, value.data()); 
}
void Material::setMat4Float(const std::string &name, mat4 value) const
{ 
    glUniformMatrix4fv(glGetUniformLocation(progID, name.c_str()), 1, GL_TRUE, value.data());
}

void Material::setupTexs() {
  for (unsigned int i = 0; i < texs.size(); i++)
  {
    unsigned int x;
    glGenTextures(1, &x);
    texIds[i] = x;

    Material::load_tex_helper(texIds[i], texs[i].path);
    // configure_texture(texIds[i], texs[i].min, texs[i].); 
  }
  std::cout << "Setup textures..." << std::endl;
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