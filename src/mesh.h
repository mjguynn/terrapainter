#pragma once

#include <optional>

#include "geometry.h"
#include "material.h"

class Mesh
{
private:
  std::optional<Geometry> mGeo;
  Material mMat;
  bool mInstanced = false;
  int mInstanceAmount = 0;

public:
  GLuint VAO;

  Mesh(Material &&mat) : mGeo(), mMat(std::move(mat))
  {
    glGenVertexArrays(1, &VAO);
  }
  Mesh(Material &&mat, Geometry &&geo)
      : Mesh(std::move(mat))
  {
    setGeometry(std::move(geo));
  }

  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

  ~Mesh() noexcept
  {
    glDeleteVertexArrays(1, &VAO);
    for (auto &vbo : VBOs)
    {
      glDeleteBuffers(1, &vbo.second);
    }
    if (EBO)
    {
      glDeleteBuffers(1, &EBO);
    }
  }

  const Material &mat() const { return mMat; }

  void setInstance(bool shouldInstance)
  {
    mInstanced = shouldInstance;
  }

  void setInstanceCount(int count)
  {
    mInstanceAmount = count;
  }

  void draw() const
  {

    if (!mGeo.has_value())
      return;
    // bind appropriate textures
    int i = 0;
    for (const auto &[tex, id] : mMat.texs)
    {
      if (mMat.uniforms().count(tex.name) != 0)
      {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, id);
        mMat.setInt(tex.name, i);
      }
      i += 1;
    }

    glBindVertexArray(VAO);
    const Geometry &geo = mGeo.value();
    if (geo.hasIndices())
    {
      switch (geo.primitive)
      {
      case GL_TRIANGLES:
        if (mInstanced)
        {
          printf("Drawing Instance with count %d\n", mInstanceAmount);
          glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(geo.indices->size()), GL_UNSIGNED_INT, 0, mInstanceAmount);
        }
        else
        {
          glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(geo.indices->size()), GL_UNSIGNED_INT, 0);
        }
        break;

      case GL_TRIANGLE_STRIP:
      {
        auto sData = geo.stripData.value();
        for (unsigned int strip = 0; strip < sData.strips; strip++)
        {
          glDrawElements(
              GL_TRIANGLE_STRIP,
              sData.numTrisPerStrip + 2,
              GL_UNSIGNED_INT,
              (void *)(sizeof(unsigned int) * (sData.numTrisPerStrip + 2) * strip));
        }

        break;
      }

      default:
        assert(0);
        break;
      }
    }
    else
    {
      // TODO
      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glBindVertexArray(0);

    // always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
  }

  void setGeometry(Geometry &&geo)
  {
    // GAH
    if (mGeo)
      mGeo->~Geometry();
    new (&mGeo) std::optional(std::move(geo));

    // printf("sanity check: (%f, %f, %f)",
    //  ((float*)this->ge["texCoord"]->data)[0], ((float*)geo.attrs["texCoord"]->data)[1], ((float*)geo.attrs["texCoord"]->data)[2]);

    // create buffers/arrays

    glBindVertexArray(VAO);

    for (const auto &it : mGeo->attrs)
    {
      if (mMat.attrs().contains(it.first))
      {
        unsigned int VBO;
        glGenBuffers(1, &VBO);
        VBOs[it.first] = VBO;
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, it.second.t_size, it.second.data, GL_STATIC_DRAW);

        // set the vertex attribute pointers
        int attribId = mMat.attrs().at(it.first);
        glEnableVertexAttribArray(attribId);
        glVertexAttribPointer(attribId, it.second.size, it.second.type, it.second.normalize, 0, (void *)0);
      }
    }

    if (mGeo->hasIndices())
    {
      glGenBuffers(1, &EBO);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, mGeo->indices->size() * sizeof(unsigned int), &mGeo->indices.value()[0], GL_STATIC_DRAW);
    }

    glBindVertexArray(0);
  }

private:
  std::unordered_map<std::string, unsigned int> VBOs;
  unsigned int EBO = 0;
};
