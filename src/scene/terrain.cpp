#include "../shadermgr.h"
#include "terrain.h"
#include <algorithm>
#include "../helpers.h"

Terrain::Terrain(vec3 position, vec3 angles, vec3 scale)
    : Entity(position, angles, scale)
{
    mGrassProgram = g_shaderMgr.geometry("grass");
    mMesh = nullptr;
    mGrassVAO = 0;
    mGrassVBO = 0;
    mNumGrassTriangles = 0;

    std::vector<Texture> mTexs;
    mTexs.push_back(Texture{"mSand", "sand.png"});
    mTexs.push_back(Texture{"mGrass", "grass.png"});
    mTexs.push_back(Texture{"mDirt", "dirt.png"});
    mTexs.push_back(Texture{"mMnt", "mountain.png"});
    mTexs.push_back(Texture{"mGrassNorm", "grass-normal.png"});
    mTexs.push_back(Texture{"mMountNorm", "mountain-normal.png"});
    mTexs.push_back(Texture{"mSandNorm", "sand-normal.png"});
    mTexs.push_back(Texture{"mSnowNorm", "snow-normal.png"});
    mTexs.push_back(Texture{"mDirtNorm", "dirt-normal.png"});

    tMat = new Material("heightmap", mTexs);

    glGenTextures(1, &mGrassTexture);
    load_mipmap_texture(mGrassTexture, "grassPack.png");
    mAlphaTest = 0.25f;
    mAlphaMultiplier = 1.5f;
}
Terrain::~Terrain() noexcept
{
    free(tMat);
    if (mGrassVAO)
        glDeleteVertexArrays(1, &mGrassVAO);
    if (mGrassVBO)
        glDeleteBuffers(1, &mGrassVBO);
    assert(mGrassTexture);
    glDeleteTextures(1, &mGrassTexture);
}

// Code adapted from: https://stackoverflow.com/questions/28889210/smoothstep-function
float smoothstep(float edge0, float edge1, float x)
{
    // Scale, bias and saturate x to 0..1 range
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    // Evaluate polynomial
    return x * x * (3 - 2 * x);
}

void Terrain::generate(const Canvas &source)
{
    // Delete existing mesh
    mMesh = nullptr;

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    auto [width, height] = source.get_canvas_size();
    if (width <= 0 || height <= 0)
    {
        // canvas not ready, don't do anything else
        return;
    }
    auto pixels = source.get_canvas();

    std::vector<float> positions;
    std::vector<vec3> grassVertices;
    float zScale = 96.0f / 256.0f, zShift = 16.0f;
    int rez = 1;
    unsigned bytePerPixel = 4;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            unsigned char *pixelOffset = (unsigned char *)pixels.data() + (j + width * i) * bytePerPixel;
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

    const int numStrips = (height - 1) / rez;
    const int numTrisPerStrip = (width / rez) * 2 - 2;
    fprintf(stderr, "[info] loaded %llu indices\n", indices.size());
    fprintf(stderr, "[info] created lattice of %i strips with %i triangles each\n", numStrips, numTrisPerStrip);
    fprintf(stderr, "[info] created %i triangles total\n", numStrips * numTrisPerStrip);

    Attribute *aPos = new Attribute(&positions, 3);
    Geometry tGeo = Geometry(height, width, numTrisPerStrip, numStrips);
    tGeo.setIndex(indices);
    tGeo.setAttr("position", aPos);
    tGeo.GenerateNormalTangent();

    mMesh = std::make_unique<Mesh>(tGeo, *this->tMat);

    // ---------------------- Grass----------------------------------------
    for (int i = 0; i < aPos->count; i++)
    {
        vec3 vPos = vec3::splat(0.0);
        aPos->getXYZ(i, vPos);
        float h = vPos.z;
        vec3 n = vec3::splat(0.0);
        tGeo.getAttr("normal")->getXYZ(i, n);

        float fGrassPatchOffsetMin = 1.0f;
        float fGrassPatchOffsetMax = 2.0f;

        float probability = 0;
        if (dot(n, vec3(0, 0, 1)) < 0.95)
        {
            probability = 0;
        }
        else if (h > 3 && h < 18)
        {
            probability = 0.25;
        }
        else
        {
            probability = 0;
        }

        if (rand() % 100 < probability * 100)
        {
            float vertOffset = 1.3;
            grassVertices.push_back(vec3(vPos.x, vPos.y, vPos.z - vertOffset));
            float value = fGrassPatchOffsetMin + (fGrassPatchOffsetMax - fGrassPatchOffsetMin) * float(rand() % 1000) * 0.001f;
            float x = vPos.x + value;
            float y = vPos.y + value;
            grassVertices.push_back(vec3(x, y, vPos.z - vertOffset));
            x = vPos.x + value;
            y = vPos.y - value;
            grassVertices.push_back(vec3(x, y, vPos.z - vertOffset));
            x = vPos.x - value;
            y = vPos.y + value;
            grassVertices.push_back(vec3(x, y, vPos.z - vertOffset));
            x = vPos.x - value;
            y = vPos.y - value;
            grassVertices.push_back(vec3(x, y, vPos.z - vertOffset));
            grassVertices.push_back(vec3(vPos.x, vPos.y + value, vPos.z - vertOffset));
            grassVertices.push_back(vec3(vPos.x, vPos.y - value, vPos.z - vertOffset));
            grassVertices.push_back(vec3(vPos.x + value, vPos.y, vPos.z - vertOffset));
            grassVertices.push_back(vec3(vPos.x - value, vPos.y, vPos.z - vertOffset));
            mNumGrassTriangles += 9;
        };
    }

    if (mGrassVAO)
        glDeleteVertexArrays(1, &mGrassVAO);
    if (mGrassVBO)
        glDeleteBuffers(1, &mGrassVBO);
    glGenVertexArrays(1, &mGrassVAO);
    glGenBuffers(1, &mGrassVBO);
    glBindVertexArray(mGrassVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mGrassVBO);
    glBufferData(GL_ARRAY_BUFFER, grassVertices.size() * sizeof(vec3), grassVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(vec3), (void *)0);
    glEnableVertexAttribArray(0);

    // -------------------------Grass (END) --------------------------------------
}
void Terrain::draw(ivec2 viewportSize, const mat4 &viewProj, vec3 viewPos, vec4 cullPlane) const
{
    // This can happen if no canvas is loaded, I suppose...
    // Not sure whether it's worth making this an error condition.
    if (!mMesh)
        return;

    const mat4 modelToWorld = world_transform();
    // TODO change this
    vec3 lightDir = {0.0f, 0.0f, -5.0f};
    glUseProgram(tMat->progID);

    mMesh->mat.setMat4Float("u_worldToProjection", viewProj);
    mMesh->mat.setMat4Float("u_modelToWorld", modelToWorld);
    mMesh->mat.set3Float("LightDir", lightDir);
    mMesh->mat.set3Float("viewPos", viewPos);
    mMesh->mat.set4Float("u_cullPlane", cullPlane);
    mMesh->mat.setBool("blinn", true);
    mMesh->Draw();

    float time = double(SDL_GetTicks64()) / 1000.0;
    glUseProgram(mGrassProgram);
    glUniformMatrix4fv(0, 1, GL_TRUE, viewProj.data());
    glUniformMatrix4fv(1, 1, GL_TRUE, modelToWorld.data());
    glUniform1f(2, time);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mGrassTexture);
    glUniform1i(4, 0);

    glUniform4f(5, 1, 1, 1, 1);
    glUniform1f(6, mAlphaTest);
    glUniform1f(7, mAlphaMultiplier);

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    glBindVertexArray(mGrassVAO);
    glDrawArrays(GL_POINTS, 0, mNumGrassTriangles);

    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDisable(GL_MULTISAMPLE);
}