#include "shadermgr.h"
#include "terrain.h"

Terrain::Terrain(vec3 position, vec3 angles, vec3 scale) 
    : Entity(position, angles, scale) 
{
    mProgram = g_shaderMgr.graphics("heightmap");
    mWorldToProjectionLocation = glGetUniformLocation(mProgram, "u_worldToProjection");
    mModelToWorldLocation = glGetUniformLocation(mProgram, "u_modelToWorld");
    mLightDirLocation = glGetUniformLocation(mProgram, "LightDir");
    mViewPosLocation = glGetUniformLocation(mProgram, "viewPos");
    mMesh = nullptr;
}
Terrain::~Terrain() noexcept {
    // nothing for now...
}
void Terrain::generate(const Canvas& source) {
    // Delete existing mesh
    mMesh = nullptr;

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    auto [width, height] = source.get_canvas_size();
    if (width <= 0 || height <= 0) {
        // canvas not ready, don't do anything else
        return;
    }
    auto pixels = source.get_canvas();

    std::vector<float> positions;
    float zScale = 96.0f / 256.0f, zShift = 16.0f;
    int rez = 1;
    unsigned bytePerPixel = 4;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            unsigned char* pixelOffset = (unsigned char*)pixels.data() + (j + width * i) * bytePerPixel;
            unsigned char z = pixelOffset[0];

            positions.push_back(-width / 2.0f + width * j / (float)width);
            positions.push_back(-height / 2.0f + height * i / (float)height);
            positions.push_back((int)z * zScale - zShift);
        }
    }
    fprintf(stderr, "[info] generated %llu vertices \n", positions.size() / bytePerPixel / 3);

    std::vector<unsigned> indices;
    for (unsigned i = 0; i < height - 1; i += rez)
    {
        for (unsigned j = 0; j < width; j += rez)
        {
            for (unsigned k = 0; k < 2; k++)
            {
                indices.push_back(j + width * (i + k * rez));
            }
        }
    }
    // ------------------- Normal(End) -----------------

    const int numStrips = (height - 1) / rez;
    const int numTrisPerStrip = (width / rez) * 2 - 2;
    fprintf(stderr, "[info] loaded %llu indices\n", indices.size());
    fprintf(stderr, "[info] created lattice of %i strips with %i triangles each\n", numStrips, numTrisPerStrip);
    fprintf(stderr, "[info] created %i triangles total\n", numStrips * numTrisPerStrip);

    Attribute* aPos = new Attribute(&positions, 3);
    Geometry tGeo = Geometry(height, width, numTrisPerStrip, numStrips);
    tGeo.setIndex(indices);
    tGeo.setAttr("position", aPos);
    tGeo.GenerateNormals();

    Material tMat = Material("heightmap");
    mMesh = std::make_unique<Mesh>(tGeo, tMat);

    std::cout << "Created terrain Mesh" << std::endl;
}
void Terrain::draw(const mat4& viewProj, vec3 viewPos) const {
    // This can happen if no canvas is loaded, I suppose...
    // Not sure whether it's worth making this an error condition.
    if (!mMesh) return;

    const mat4 modelToWorld = world_transform();
    vec3 lightDir = { 0.0f, 0.0f, -5.0f };
    glUseProgram(mProgram);
    glUniformMatrix4fv(mWorldToProjectionLocation, 1, GL_TRUE, viewProj.data());
    glUniformMatrix4fv(mModelToWorldLocation, 1, GL_TRUE, modelToWorld.data());
    glUniform3f(mLightDirLocation, lightDir.x, lightDir.y, lightDir.z);
    glUniform3f(mViewPosLocation, viewPos.x, viewPos.y, viewPos.z);
    //std::cout << "Drawing Terrain..." << std::endl;
    mMesh->Draw();
}