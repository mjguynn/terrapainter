#include "terrapainter/material.h"
#include "shadermgr.h"

#include "vector"
#include "stb/stb_image.h"
#include <iostream>
#include "shadermgr.h"
#include <algorithm>

Material::Material(std::string shaderName, std::vector<Texture> texs) {
  progID = g_shaderMgr.graphics(shaderName);
  g_shaderMgr.mPrograms[shaderName].rebuild();
  attrLocs = g_shaderMgr.mPrograms[shaderName].attrLocs;
  uniformLocs = g_shaderMgr.mPrograms[shaderName].uniformLocs;

  texs = texs;
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

void Material::setupTexs() {
  for (unsigned int i = 0; i < texs.size(); i++)
  {
    glGenTextures(1, &texIds[i]);
    glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding

    // TODO: Could parameterize options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char *data = stbi_load((texs[i].path).c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    glBindTexture(GL_TEXTURE_2D, texIds[i]);
  }
  glActiveTexture(GL_TEXTURE0);
  std::cout << "Setup textures..." << std::endl;
}