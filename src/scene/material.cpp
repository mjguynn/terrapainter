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

    load_mipmap_texture(texIds[i], texs[i].path);
    // configure_texture(texIds[i], texs[i].min, texs[i].); 
  }
  std::cout << "Setup textures..." << std::endl;
}