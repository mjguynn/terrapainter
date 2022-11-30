#include <string>
#include <vector>
#include <unordered_map>
#include <glad/gl.h>

struct Texture {
    std::string name;
    std::string path;
};

class Material {
  private:
    std::vector<Texture> texs;
    GLuint progID;
  public:
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
  private:
    void setupTexs();
};