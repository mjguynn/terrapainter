#pragma once

#include <vector>
#include "terrapainter/math.h"

// Abstraction of a vertex attribute passed into some location in a vertex shader
// Can pass in float, uint, or ubyte
class Attribute {
private:  
public:
  uint8_t* data;
  // Number of components of array per generic vertex attribute
  unsigned int size;
  // Total size of data in bytes
  unsigned int t_size;
  // OpenGL type passed into attribute
  unsigned int type;
  int normalize;
  // Number of entries 
  unsigned int count;
  bool needsUpdate = true;

  Attribute(std::vector<float>* contents, unsigned int size) {
    t_size = contents->size() * sizeof(float);

    auto d = (float*)malloc(t_size);
    std::copy(contents->begin(), contents->end(), d);
    data = (uint8_t*)d;

    this->size = size;
    count = contents->size() / size; 
    type = 0x1406;
    normalize = 0;
  }
  Attribute(std::vector<unsigned int>* contents, unsigned int size) {
    t_size = contents->size() * sizeof(unsigned int);

    data = (uint8_t*)malloc(t_size);
    std::copy(contents->begin(), contents->end(), data);

    this->size = size;
    count = contents->size() / size; 
    type = 0x1405;
    normalize = 0;
  }
  // Unsigned byte, mapped to [0, 1]
  Attribute(std::vector<signed char>* contents, unsigned int size) {
    t_size = contents->size() * sizeof(signed char);

    data = (uint8_t*)malloc(t_size);
    std::copy(contents->begin(), contents->end(), data);
    this->size = size;
    count = contents->size() / size; 
    type = 0x1401;
    normalize = 1;
  }
  
  template <typename T> 
  void setXYZ(unsigned int index, T x, T y, T z) {
    index *= size;
    if ((index + 3) * sizeof(T) > t_size) {
      printf("(index + 3) * sizeof(T): %d, t_size: %d \n", (index + 3) * sizeof(T), t_size);
    }
    assert((index + 3) * sizeof(T) <= t_size);
    *(T*)(data + (index) * sizeof(T)) = x;
    *(T*)(data + (index+1) * sizeof(T)) = y;
    *(T*)(data + (index+2) * sizeof(T)) = z;
  }

  template <typename T>
  void getXYZ(unsigned int index, math::MVector<T, 3>& v) {
    index *= size;
    if ((index + 3) * sizeof(T) > t_size) {
      printf("(index + 3) * sizeof(T): %d, t_size: %d \n", (index + 3) * sizeof(T), t_size);
    }
    assert((index + 3) * sizeof(T) <= t_size);
    v.x = *(T*)(data + (index) * sizeof(T));
    v.y = *(T*)(data + (index+1) * sizeof(T));
    v.z = *(T*)(data + (index+2) * sizeof(T));
  }
  template <typename T>
  void getXY(unsigned int index, math::MVector<T, 2>& v) {
    index *= size;
    assert((index + 2) * sizeof(T) > t_size);
    v.x = *(T*)(data + (index) * sizeof(T));
    v.y = *(T*)(data + (index+1) * sizeof(T));
  }
  template <typename T>
  void getX(unsigned int index, T& v) {
    index *= size;
    assert((index + 1) * sizeof(T) > t_size);
    v = *(T*)(data + (index) * sizeof(T));
  }

  // private:
  //   static unsigned int GetSizeOfType(unsigned int type);
};