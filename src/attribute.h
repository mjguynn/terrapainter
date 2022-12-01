#pragma once

#include <vector>
#include <glad/gl.h>
#include "terrapainter/math.h"
#include <cstring>

// Abstraction of a vertex attribute passed into some location in a vertex shader
// Can pass in float, uint, or ubyte
class Attribute
{
private:
public:
  uint8_t *data;
  // Number of components of array per generic vertex attribute
  GLuint size;
  // Total size of data in bytes
  GLuint t_size;
  // OpenGL type passed into attribute
  GLenum type;
  GLboolean normalize;
  // Number of entries
  GLenum count;

  Attribute(std::vector<GLfloat> *contents, GLuint size)
  {
    t_size = contents->size() * sizeof(GLfloat);

    data = (uint8_t *)malloc(t_size);
    memcpy(data, contents->data(), t_size);

    this->size = size;
    count = contents->size() / size;
    type = GL_FLOAT;
    normalize = GL_FALSE;
  }
  Attribute(std::vector<GLuint> *contents, GLuint size)
  {
    t_size = contents->size() * sizeof(GLuint);

    data = (uint8_t *)malloc(t_size);
    memcpy(data, contents->data(), t_size);

    this->size = size;
    count = contents->size() / size;
    type = GL_UNSIGNED_INT;
    normalize = GL_FALSE;
  }
  // Unsigned byte, mapped to [0, 1]
  Attribute(std::vector<GLubyte> *contents, GLuint size)
  {
    t_size = contents->size() * sizeof(GLubyte);

    data = (uint8_t *)malloc(t_size);
    memcpy(data, contents->data(), t_size);

    this->size = size;
    count = contents->size() / size;
    type = GL_UNSIGNED_BYTE;
    normalize = GL_TRUE;
  }

  Attribute(const Attribute &) = delete;
  Attribute &operator=(const Attribute &) = delete;
  Attribute(Attribute &&other) noexcept
      : data(other.data),
        size(other.size),
        t_size(other.t_size),
        type(other.type),
        normalize(other.normalize),
        count(other.count)
  {
    other.data = nullptr;
  }
  ~Attribute() noexcept
  {
    if (data)
      free(data);
  }
  // set contents at index as x, y, z
  template <typename T>
  void setXYZ(unsigned int index, T x, T y, T z)
  {
    index *= size;
    if ((index + 3) * sizeof(T) > t_size)
    {
      fprintf(stderr, "[debug] (index + 3) * sizeof(T): %llu, t_size: %u \n", (index + 3) * sizeof(T), t_size);
    }
    assert((index + 3) * sizeof(T) <= t_size);
    *(T *)(data + (index) * sizeof(T)) = x;
    *(T *)(data + (index + 1) * sizeof(T)) = y;
    *(T *)(data + (index + 2) * sizeof(T)) = z;
  }

  // dump contents at index in v
  template <typename T>
  void getXYZ(unsigned int index, math::MVector<T, 3> &v)
  {
    index *= size;
    if ((index + 3) * sizeof(T) > t_size)
    {
      fprintf(stderr, "[debug] (index + 3) * sizeof(T): %llu, t_size: %u \n", (index + 3) * sizeof(T), t_size);
    }
    assert((index + 3) * sizeof(T) <= t_size);
    v.x = *(T *)(data + (index) * sizeof(T));
    v.y = *(T *)(data + (index + 1) * sizeof(T));
    v.z = *(T *)(data + (index + 2) * sizeof(T));
  }
  // dump contents at index in v
  template <typename T>
  void getXY(unsigned int index, math::MVector<T, 2> &v)
  {
    index *= size;
    assert((index + 2) * sizeof(T) > t_size);
    v.x = *(T *)(data + (index) * sizeof(T));
    v.y = *(T *)(data + (index + 1) * sizeof(T));
  }
  // dump contents at index in v
  template <typename T>
  void getX(unsigned int index, T &v)
  {
    index *= size;
    assert((index + 1) * sizeof(T) > t_size);
    v = *(T *)(data + (index) * sizeof(T));
  }
  // private:
  //   static unsigned int GetSizeOfType(unsigned int type);
};