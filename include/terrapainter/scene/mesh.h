#pragma once

#include "terrapainter/scene/geometry.h"
#include "terrapainter/scene/material.h"
#include "entity.h"

class Mesh : Entity
{
public:
  Geometry geo;
  Material mat;
  unsigned int VAO;
  Mesh(Geometry geo, Material mat, vec3 position = vec3::zero(), vec3 angles = vec3::zero(), vec3 scale = vec3::splat(1.f)) : Entity(position, angles, scale), mat(mat)
  {
    this->geo = geo;
    this->mat = mat;
    setupMesh();
  }

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

  void Draw()
  {
    // bind appropriate textures
    for (unsigned int i = 0; i < mat.texNames.size(); i++)
    {
      assert(mat.uniformLocs.contains(mat.texNames[i]));
      glActiveTexture(GL_TEXTURE0 + i);
      // now set the sampler to the correct texture unit
      mat.setInt(mat.texNames[i], i);
      // and finally bind the texture
      glBindTexture(GL_TEXTURE_2D, mat.texIds[i]);
    }

    glBindVertexArray(VAO);

    if (geo.hasIndices())
    {
      switch (geo.primitive)
      {
      case GL_TRIANGLES:
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(geo.indices->size()), GL_UNSIGNED_INT, 0);
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

private:
  std::unordered_map<std::string, unsigned int> VBOs;
  unsigned int EBO = 0;

  void setupMesh()
  {
    // printf("sanity check: (%f, %f, %f)",
    //  ((float*)geo.attrs["tangent"]->data)[0], ((float*)geo.attrs["tangent"]->data)[1], ((float*)geo.attrs["tangent"]->data)[2]);

    // create buffers/arrays
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    for (auto &it : geo.attrs)
    {
      if (mat.attrLocs.contains(it.first))
      {
        unsigned int VBO;
        glGenBuffers(1, &VBO);
        VBOs[it.first] = VBO;
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, it.second->t_size, it.second->data, GL_STATIC_DRAW);

        // set the vertex attribute pointers
        int attribId = mat.attrLocs[it.first];
        glEnableVertexAttribArray(attribId);
        glVertexAttribPointer(attribId, it.second->size, it.second->type, it.second->normalize, 0, (void *)0);
      }
    }

    if (geo.hasIndices())
    {
      glGenBuffers(1, &EBO);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, geo.indices->size() * sizeof(unsigned int), &geo.indices.value()[0], GL_STATIC_DRAW);
    }

    glBindVertexArray(0);
  }
};
