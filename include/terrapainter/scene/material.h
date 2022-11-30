#include <string>
#include <vector>
#include <unordered_map>
#include <glad/gl.h>
#include "terrapainter/math.h"

struct Texture {
    std::string name;
    std::string path;
    GLenum min = GL_LINEAR_MIPMAP_LINEAR;
    GLenum mag = GL_LINEAR;
    GLenum sWrap = GL_REPEAT;
    GLenum tWrap = GL_REPEAT;
    GLenum internalFormat = GL_RGB;
    GLenum abstractFormat = GL_RGB;
    GLenum type = GL_UNSIGNED_BYTE;
};

class Material {
  private:
    std::vector<Texture> texs;
  public:
    GLuint progID;
    std::vector<std::string> texNames;
    std::vector<unsigned int> texIds;
    std::unordered_map<std::string, int> attrLocs;
    std::unordered_map<std::string, int> uniformLocs;

    Material(std::string shaderName);
    Material(std::string shaderName, std::vector<Texture> texs);

    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const;
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const;
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const;
    void set3Float(const std::string &name, vec3 value) const;
    void set4Float(const std::string &name, vec4 value) const;
    void setMat4Float(const std::string &name, mat4 value) const;
  private:
    void setupTexs();
};