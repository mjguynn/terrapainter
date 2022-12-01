#pragma once

#include <vector>
#include <unordered_map>
#include <optional>
#include <glad/gl.h>

#include "attribute.h"
#include "terrapainter/math.h"

struct StripData {
  int height;
  int width;
  int numTrisPerStrip;
  int strips;
};

class Geometry {
  public:
    // map from names (case sensitive) to attributes
    std::unordered_map<std::string, Attribute> attrs;
    std::optional<std::vector<GLuint>> indices;
    GLint primitive;
    std::optional<StripData> stripData;

    // Triangle primitive
    Geometry();
    // Triangle Strip primitive
    Geometry(int height, int width, int numTrisPerStrips, int strips);

    Geometry(const Geometry&) = delete;
    Geometry& operator=(const Geometry&) = delete;

    Geometry(Geometry&&) noexcept;

    void setAttr(std::string name, Attribute&& attr);
    void setIndex(std::vector<GLuint> idx);
    bool hasIndices() const;

    // Returns NULL if not found
    Attribute* getAttr(std::string name);
    bool hasAttr(std::string name) const;

    // Expects an attribute named "normal"
    void normalizeNormals();

    // Expects an attribute named "tangent"
    void normalizeTangents();

    // Expects an attribute named "position"
    // Creates new attribute named "normal" containing normals for each position
    // Creates new attribute named "tangent" containing normals for each position
    // Normals are generated differently depending on the primitive, and if indices are present
    void GenerateNormalTangent();

    private:
    void GenerateNormalTangentTriangle();
    void GenerateNormalTangentStrips();

};