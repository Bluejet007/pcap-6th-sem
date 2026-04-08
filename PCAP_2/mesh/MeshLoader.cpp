#include "MeshLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <string>
void processMesh(aiMesh* mesh,
                 std::vector<Vertex>& vertices,
                 std::vector<unsigned int>& indices,
                 glm::vec3& minB,
                 glm::vec3& maxB) {

    unsigned int offset = vertices.size();

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex v;

        v.Position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        if (mesh->HasNormals()) {
            v.Normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }

        minB = glm::min(minB, v.Position);
        maxB = glm::max(maxB, v.Position);

        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(offset + face.mIndices[j]);
        }
    }
}

void processNode(aiNode* node,
                 const aiScene* scene,
                 std::vector<Vertex>& vertices,
                 std::vector<unsigned int>& indices,
                 glm::vec3& minB,
                 glm::vec3& maxB) {

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, vertices, indices, minB, maxB);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene,
                    vertices, indices, minB, maxB);
    }
}

void loadMesh(const std::string& path,
              std::vector<Vertex>& vertices,
              std::vector<unsigned int>& indices,
              glm::vec3& center) {

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals
    );

    if (!scene || !scene->mRootNode) {
        std::cout << "ERROR loading mesh\n";
        return;
    }

    glm::vec3 minB(1e10f), maxB(-1e10f);

    processNode(scene->mRootNode, scene,
                vertices, indices, minB, maxB);

    center = (minB + maxB) * 0.5f;
}