#pragma once 

#include <glad/gl.h> 

#include <stb/stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <terrapainter/scene/mesh.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

class Model 
{
public:
    vector<Mesh*> meshes;
    string directory;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, std::string shaderName)
    {
        loadModel(path, shaderName);
    }

    ~Model() {
      for (auto &m : meshes)
        free(m);
    }

    // draws the model, and thus all its meshes
    void Draw()
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i]->Draw();
    }
    
private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path, std::string shaderName)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
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
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, shaderName));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene, shaderName);
        }

    }

    Mesh* processMesh(aiMesh *mesh, const aiScene *scene, std::string shaderName)
    {
        // data to fill
        vector<float> positions;
        vector<float> normals;
        vector<float> texCoords;
        vector<float> tangents;
        vector<float> biTangents;

        vector<unsigned int> indices;
        vector<Texture> textures;

        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            positions.push_back(mesh->mVertices[i].x);
            positions.push_back(mesh->mVertices[i].y);
            positions.push_back(mesh->mVertices[i].z);

            // normals
            if (mesh->HasNormals())
            {
              normals.push_back(mesh->mNormals[i].x);
              normals.push_back(mesh->mNormals[i].y);
              normals.push_back(mesh->mNormals[i].z);
            }

            // texture coordinates
            if(mesh->mTextureCoords[0])
            {
              texCoords.push_back(mesh->mTextureCoords[0][i].x);
              texCoords.push_back(mesh->mTextureCoords[0][i].y);
              
              tangents.push_back(mesh->mTangents[i].x);
              tangents.push_back(mesh->mTangents[i].y);
              tangents.push_back(mesh->mTangents[i].z);

              biTangents.push_back(mesh->mBitangents[i].x);
              biTangents.push_back(mesh->mBitangents[i].y);
              biTangents.push_back(mesh->mBitangents[i].z);
            }
            else {
              texCoords.push_back(0.0);
              texCoords.push_back(0.0);
            }
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        Geometry mGeo = Geometry();
        //TODO: AADD CHECKS
        Attribute* aPos = new Attribute(&positions, 3);
        Attribute* aNormals = new Attribute(&normals, 3);
        Attribute* atexCoords = new Attribute(&texCoords, 3);
        Attribute* aTangents = new Attribute(&tangents, 3);
        Attribute* aBiTangents = new Attribute(&biTangents, 3);

        mGeo.setAttr("position", aPos);
        mGeo.setAttr("normal", aNormals);
        mGeo.setAttr("texCoord", atexCoords);
        mGeo.setAttr("tangent", aTangents);
        mGeo.setAttr("biTangent", aBiTangents);


        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
        Material mMat = Material(shaderName, textures);
        Mesh* nMesh = new Mesh(mGeo, mMat);

        // return a mesh object created from the extracted mesh data
        return nMesh;
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            // if texture hasn't been loaded already, load it
            Texture texture;
            std::string filename = this->directory + '/' + string(str.C_Str());
            texture.path = filename;
            texture.name = typeName + to_string(i);
            textures.push_back(texture);
            
        }
        return textures;
    }
};