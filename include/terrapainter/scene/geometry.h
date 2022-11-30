#pragma once

#include <vector>
#include <glad/gl.h>
#include <unordered_map>
#include <terrapainter/scene/attribute.h>
#include <terrapainter/math.h>
#include <optional>

struct StripData {
  int height;
  int width;
  int numTrisPerStrip;
  int strips;
};

class Geometry {
  public:
    // map from names (case sensitive) to attributes
    std::unordered_map<std::string, Attribute*> attrs;
    std::optional<std::vector<unsigned int>> indices;
    GLint primitive;
    std::optional<StripData> stripData;

    // Triangle primitive
    Geometry();
    // Triangle Strip primitive
    Geometry(int height, int width, int numTrisPerStrips, int strips);

    void setAttr(std::string name, Attribute* attr);
    void setIndex(std::vector<unsigned int> idx);
    bool hasIndices();

    // Returns NULL if not found
    Attribute* getAttr(std::string name);
    bool hasAttr(std::string name);

    // Expects an attribute named "normal"
    void normalizeNormals();

    // Expects an attribute named "position"
    // Creates new attribute named "normal" containing normals for each position
    // Normals are generated differently depending on the primitive, and if indices are present
    void GenerateNormals();

    private:
    void GenerateNormalTriangle();
    void GenerateNormalStrips();

};