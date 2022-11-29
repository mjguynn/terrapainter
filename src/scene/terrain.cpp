#include "../shadermgr.h"
#include "terrain.h"

Terrain::Terrain(vec3 position, vec3 angles, vec3 scale) 
    : Entity(position, angles, scale) 
{
    mProgram = g_shaderMgr.graphics("heightmap");
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

    std::vector<Vertex> vertices;
    float zScale = 96.0f / 256.0f, zShift = 16.0f;
    int rez = 1;
    unsigned bytePerPixel = 4;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            unsigned char* pixelOffset = (unsigned char*)pixels.data() + (j + width * i) * bytePerPixel;
            unsigned char z = pixelOffset[0];

            vertices.push_back(
                Vertex{
                    .Position = vec3(-width / 2.0f + width * j / (float)width, -height / 2.0f + height * i / (float)height, (int)z * zScale - zShift)
                }
            );
        }
    }
    fprintf(stderr, "[info] generated %llu vertices \n", vertices.size() / bytePerPixel);

    // ------------------ Normal (start)-------------------------

    // facedata[i] is the vertex index for face i // 3
    std::vector<unsigned int> facedata;
    // loading each face in
    for (int i = 0; i < height - 1; i++)
    {
        for (int j = 0; j < width - 1; j++)
        {
            facedata.push_back(i * width + j);
            facedata.push_back(i * width + j + 1);
            facedata.push_back((i + 1) * width + j);
            facedata.push_back(i * width + j + 1);
            facedata.push_back((i + 1) * width + j + 1);
            facedata.push_back((i + 1) * width + j);
        }
    }

    // normal[i] is the vec3 normal of vertices[i]
    std::vector<vec3> normaldata;
    for (int i = 0; i < vertices.size(); i++)
    {
        normaldata.push_back(vec3(0));
    }

    for (int i = 0; i < facedata.size(); i += 3)
    {
        vec3 v1 = vertices.at(facedata.at(i)).Position;
        vec3 v2 = vertices.at(facedata.at(i + 1)).Position;
        vec3 v3 = vertices.at(facedata.at(i + 2)).Position;

        vec3 side1 = v2 - v1;
        vec3 side2 = v3 - v1;
        vec3 normal = cross(side1, side2);

        normaldata[facedata.at(i)] += normal;
        normaldata[facedata.at(i + 1)] += normal;
        normaldata[facedata.at(i + 2)] += normal;
    }


    for (int i = 0; i < normaldata.size(); i += 1)
    {
        normaldata[i] = normaldata[i].normalize();
        vertices[i].Normal = normaldata[i];
    }

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

    mMesh = std::make_unique<Mesh>(vertices, indices, numTrisPerStrip, numStrips);
}
void Terrain::draw(const mat4& viewProj, vec3 viewPos, vec4 cullPlane) const {
    // This can happen if no canvas is loaded, I suppose...
    // Not sure whether it's worth making this an error condition.
    if (!mMesh) return;

    const mat4 modelToWorld = world_transform();
    // TODO change this
    vec3 lightDir = { 0.0f, 0.0f, -5.0f };
    glUseProgram(mProgram);
    glUniformMatrix4fv(0, 1, GL_TRUE, viewProj.data());
    glUniformMatrix4fv(1, 1, GL_TRUE, modelToWorld.data());
    glUniform3fv(2, 1, lightDir.data());
    glUniform3fv(3, 1, viewPos.data());
    glUniform4fv(4, 1, cullPlane.data());
    mMesh->DrawStrips();
}