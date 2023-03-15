#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "entity.h"
#include "mesh.h"

class Model : public Entity {
public:
    Model(std::string name, const char* path)
        : Entity(name)
    {
        loadModel(path);
    }
    void Draw(Shader& shader);

    bool isRefractive = false;

    std::vector<Mesh> meshes;
    std::vector<Texture> textures_loaded;

private:
    // model data
    std::string directory;
    TexturePackingCombination texture_packing_combination = TexturePackingCombination::NONE;

    void loadModel(std::string path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
        std::string typeName);
};

#endif
