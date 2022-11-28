#include <glad/glad.h>
#include <glm/glm.hpp>
#include "stb/stb_image.h"

#include <string>
#include <iostream>

class Texture
{
private:
  GLuint id;
  int width;
  int height;
  unsigned int type;

public:
  Texture(const char *fileName, GLenum type)
  {
    this->type = type;
    glGenTextures(1, &this->id);

    unsigned char *image = stbi_load(fileName, &this->width, &this->height, NULL, SOIL_LOAD_RGBA);

    if (image)
    {
      GLenum format;
      if (nrComponents == 1)
        format = GL_RED;
      else if (nrComponents == 3)
        format = GL_RGB;
      else if (nrComponents == 4)
        format = GL_RGBA;

      glBindTexture(type, this->id);

      glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      stbi_set_flip_vertically_on_load(true);
      glTexImage2D(type, 0, format, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
      glGenerateMipmap(type);
    }
    else
    {
      std::cout << "ERROR::TEXTURE::TEXTURE_LOADING_FAILED: " << fileName << "\n";
    }

    glActiveTexture(0);
    glBindTexture(type, 0);
    stbi_image_free(image);
  }

  ~Texture()
  {
    glDeleteTextures(1, &this->id);
  }

  inline GLuint getID() const { return this->id; }

  void bind(const GLint texture_unit)
  {
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(this->type, this->id);
  }

  void unbind()
  {
    glActiveTexture(0);
    glBindTexture(this->type, 0);
  }
};