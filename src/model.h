#pragma once

#include <glad/gl.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <unordered_map>

using namespace std;

// Adapted from https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/model.h

class Model
{
public:
    std::vector<std::pair<Texture, GLuint>> textures_loaded;
    vector<Mesh *> meshes;
    string directory;
    Material* mMat;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, std::string shaderName)
    {
        loadModel(path, shaderName);
    }

    ~Model()
    {
        for (auto &m : meshes)
            free(m);
    }

    // draws the model, and thus all its meshes
    void Draw() const
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i]->draw();
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path, std::string shaderName)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(
            path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_FlipWindingOrder );
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene, shaderName);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene, std::string shaderName)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene.
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, shaderName));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene, shaderName);
        }
    }

    Mesh *processMesh(aiMesh *mesh, const aiScene *scene, std::string shaderName)
    {
        // printf("creating mesh\n");
        // data to fill
        vector<float> positions;
        vector<float> normals;
        vector<float> texCoords;

        vector<float> tangents;
        vector<float> biTangents;

        vector<unsigned int> indices;
        vector<initTex> textures;

        // walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            positions.push_back(mesh->mVertices[i].x);
            positions.push_back(mesh->mVertices[i].y);
            positions.push_back(mesh->mVertices[i].z);

            // normals
            if (mesh->HasNormals()) {
                normals.push_back(mesh->mNormals[i].x);
                normals.push_back(mesh->mNormals[i].y);
                normals.push_back(mesh->mNormals[i].z);
            }

            // texture coordinates
            if (mesh->mTextureCoords[0])
            {
                texCoords.push_back(mesh->mTextureCoords[0][i].x);
                texCoords.push_back(mesh->mTextureCoords[0][i].y);

                // printf("texCoords for vertex %d: (%f, %f)\n", i, mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

                tangents.push_back(mesh->mTangents[i].x);
                tangents.push_back(mesh->mTangents[i].y);
                tangents.push_back(mesh->mTangents[i].z);

                biTangents.push_back(mesh->mBitangents[i].x);
                biTangents.push_back(mesh->mBitangents[i].y);
                biTangents.push_back(mesh->mBitangents[i].z);
            }
            else
            {
                texCoords.push_back(0.0);
                texCoords.push_back(0.0);
            }
        }
        // now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        // process materials
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        Geometry mGeo = Geometry();
        mGeo.setIndex(indices);

        mGeo.setAttr("position", Attribute(&positions, 3));
        mGeo.setAttr("normal", Attribute(&normals, 3));
        mGeo.setAttr("texCoord", Attribute(&texCoords, 2));
        mGeo.setAttr("tangent", Attribute(&tangents, 3));
        mGeo.setAttr("biTangent", Attribute(&biTangents, 3));

        // 1. diffuse maps
        vector<initTex> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<initTex> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<initTex> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. opacity maps
        std::vector<initTex> heightMaps = loadMaterialTextures(material, aiTextureType_OPACITY, "texture_opacity");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        Mesh *nMesh = new Mesh(Material(shaderName, textures));

        for (auto && [t, id] : nMesh->mat().texs) {
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++) {
                if(std::strcmp(textures_loaded[j].first.path.data(), t.path.data()) == 0)
                    skip = true;
                    break;
            }
            if (!skip)
                textures_loaded.emplace_back(t, id);
        }

        nMesh->setGeometry(std::move(mGeo));

        // printf("mesh created\n");

        // return a mesh object created from the extracted mesh data
        return nMesh;
    }

    vector<initTex> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<initTex> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            std::string filename = this->directory + '/' + string(str.C_Str());

            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].first.path.data(), filename.c_str()) == 0)
                {
                    // found texture with same filepath
                    textures.push_back( initTex {textures_loaded[j].second, textures_loaded[j].first} );
                    printf("found texture with same path: %s\n", textures_loaded[j].first.path.c_str());
                    skip = true;
                    break;
                }
            }
            if(!skip)
            {
                // Texture has not been loaded in yet
                initTex texture;
                texture.tex.path = filename;
                texture.tex.name = typeName + to_string(i);
                texture.id = 0;
                printf("loading texture %s with name: %s\n", texture.tex.path.c_str(), texture.tex.name.c_str());
                textures.push_back(texture);
            }
        }
        return textures;
    }
};