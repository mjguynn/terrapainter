#include <algorithm>
#include <array>
#include "terrain.h"
#include "../helpers.h"
#include "../material.h"

static std::array<Texture, 9> textures = {
    Texture{"mSand", "textures/sand.png"},
    Texture{"mGrass", "textures/grass.png"},
    Texture{"mDirt", "textures/dirt.png"},
    Texture{"mMnt", "textures/mountain.png"},
    Texture{"mGrassNorm", "textures/grass-normal.png"},
    Texture{"mMountNorm", "textures/mountain-normal.png"},
    Texture{"mSandNorm", "textures/sand-normal.png"},
    Texture{"mSnowNorm", "textures/snow-normal.png"},
    Texture{"mDirtNorm", "textures/dirt-normal.png"}};
Terrain::Terrain(vec3 position, vec3 angles, vec3 scale)
    : Entity(position, angles, scale),
      mGrassProgram(g_shaderMgr.geometry("grass")),
      mHeightmap(Material("heightmap", std::span(textures))),
      mTree(Model("models/tree/tree1low.obj", "tree"))
{
    mGrassProgram = g_shaderMgr.geometry("grass");
    glGenVertexArrays(1, &mGrassVAO);
    glGenBuffers(1, &mGrassVBO);
    mNumGrassTriangles = 0;

    mTreeProgram = g_shaderMgr.graphics("tree");

    glGenTextures(1, &mGrassTexture);
    load_mipmap_texture(mGrassTexture, "grassPack.png");
    mAlphaTest = 0.25f;
    mAlphaMultiplier = 1.5f;

    // --------- TODO HACK
    glGenVertexArrays(1, &mSeafloorVAO);
    glGenBuffers(1, &mSeafloorVBO);
    glBindVertexArray(mSeafloorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mSeafloorVBO);
    float huge = 8192.0f;
    float h = -16.0f;
    static float SEAFLOOR_VERTS[] = {
        // POSITION (XYZ)		// NORMAL (XYZ)     // TANGENT (XYZ)
        -huge, huge, h, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,  // no autoformat
        -huge, -huge, h, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, // no autoformat
        huge, huge, h, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,   // this formatting is intentional!
        huge, -huge, h, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f   // wfdasdsag
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(SEAFLOOR_VERTS), SEAFLOOR_VERTS, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (const GLvoid *)(0));
    glEnableVertexAttribArray(0); // POSITION
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (const GLvoid *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1); // NORMAL
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (const GLvoid *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2); // TANGENT
}
Terrain::~Terrain() noexcept
{
    assert(mGrassVAO);
    glDeleteVertexArrays(1, &mGrassVAO);
    assert(mGrassVBO);
    glDeleteBuffers(1, &mGrassVBO);
    assert(mGrassTexture);
    glDeleteTextures(1, &mGrassTexture);
    assert(mSeafloorVAO);
    glDeleteVertexArrays(1, &mSeafloorVAO);
    assert(mSeafloorVBO);
    glDeleteBuffers(1, &mSeafloorVBO);
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

    std::vector<GLuint> indices;
    for (GLuint i = 0; i < height - 1; i += rez)
    {
        for (GLuint j = 0; j < width; j += rez)
        {
            for (GLuint k = 0; k < 2; k++)
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

    Geometry tGeo(height, width, numTrisPerStrip, numStrips);
    tGeo.setIndex(indices);
    tGeo.setAttr("position", Attribute(&positions, 3));
    tGeo.GenerateNormalTangent();

    // ---------------------- Grass----------------------------------------
    for (int i = 0; i < tGeo.getAttr("position")->count; i++)
    {
        vec3 vPos = vec3::splat(0.0);
        tGeo.getAttr("position")->getXYZ(i, vPos);
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
            float vertOffset = 0.8;
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

    glBindVertexArray(mGrassVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mGrassVBO);
    glBufferData(GL_ARRAY_BUFFER, grassVertices.size() * sizeof(vec3), grassVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(vec3), (void *)0);
    glEnableVertexAttribArray(0);

    // -------------------------Grass (END) --------------------------------------

    // -------------------------Instancing ------------------------------------
    unsigned int amount = 10;
    mat4 *modelMatrices;
    modelMatrices = new mat4[amount];
    float offset = 25.0f;

    unsigned int cur = 0;

    for (int i = 0; i < tGeo.getAttr("position")->count; i++)
    {
        if (cur >= 10)
        {
            break;
        }
        vec3 vPos = vec3::splat(0.0);
        tGeo.getAttr("position")->getXYZ(i, vPos);
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
            probability = 0.00001;
        }
        else
        {
            probability = 0;
        }

        if (rand() % 100 < probability * 100)
        {
            float scaleFactor = static_cast<float>((rand() % 100) / 1000.0 + 0.1);
            mat4 scale = mat3::scale(scaleFactor * 0.025).hmg();

            mat4 rotation = mat3{
                1.0, 0.0, 0.0,
                0.0, 0.0, 1.0,
                0.0, 1.0, 0.0}
                                .hmg();

            mat4 translation = mat4::translate_hmg(vPos);
            mat4 model = translation * rotation * scale;

            model = model.transpose();

            std::cout << model << std::endl;

            printf("adding matrix %d\n", cur);
            // 4. now add to list of matrices
            modelMatrices[cur] = model;
            cur++;
        };
    }

    mHeightmap.setGeometry(std::move(tGeo));

    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, amount * sizeof(mat4), &modelMatrices[0], GL_STATIC_DRAW);

    for (unsigned int i = 0; i < mTree.meshes.size(); i++)
    {
        unsigned int VAO = mTree.meshes[i]->VAO;
        glBindVertexArray(VAO);
        // set attribute pointers for matrix (4 times vec4)
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void *)0);
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void *)(sizeof(vec4)));
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void *)(2 * sizeof(vec4)));
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void *)(3 * sizeof(vec4)));

        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);
        glVertexAttribDivisor(7, 1);
        glVertexAttribDivisor(8, 1);

        glBindVertexArray(0);
    }
    mTree.setInstance(amount);
}
void Terrain::draw(const RenderCtx &c) const
{
    const mat4 modelToWorld = world_transform();

    // Tree
    glUseProgram(mTreeProgram->id());
    glUniformMatrix4fv(0, 1, GL_TRUE, c.viewProj.data());
    glUniform3fv(2, 1, c.sunDir.data());
    glUniform3fv(3, 1, c.viewPos.data());
    glUniform3fv(4, 1, c.sunColor.data());

    mTree.Draw();

    // Terrain
    {
        // TODO HACK: This should really be fixed inside the mesh generation itself!
        // Instead we have to do this stupid leaky abstraction hack...
        glFrontFace(c.inWaterPass ? GL_CCW : GL_CW);
        glUseProgram(mHeightmap.mat().id());
        mHeightmap.mat().setMat4Float("u_worldToProjection", c.viewProj);
        mHeightmap.mat().setMat4Float("u_modelToWorld", modelToWorld);
        mHeightmap.mat().set3Float("u_sunDir", c.sunDir);
        mHeightmap.mat().set3Float("u_sunColor", c.sunColor);
        mHeightmap.mat().set3Float("u_viewPos", c.viewPos);
        mHeightmap.mat().set4Float("u_cullPlane", c.cullPlane);
        mHeightmap.draw();
        glFrontFace(c.inWaterPass ? GL_CW : GL_CCW);
        // the biggest hack of all time, super unstable, awful, etc
        // this depends on the shader state being the same since the previous invocation
        glBindVertexArray(mSeafloorVAO);
        mHeightmap.mat().setMat4Float("u_modelToWorld", modelToWorld);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // Grass
    if (!c.inWaterPass)
    {
        float time = double(SDL_GetTicks64()) / 1000.0;
        glUseProgram(mGrassProgram->id());
        glUniformMatrix4fv(0, 1, GL_TRUE, c.viewProj.data());
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
        glDisable(GL_CULL_FACE);

        glBindVertexArray(mGrassVAO);
        glDrawArrays(GL_POINTS, 0, mNumGrassTriangles);

        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        glDisable(GL_MULTISAMPLE);
        glEnable(GL_CULL_FACE);
    }
}