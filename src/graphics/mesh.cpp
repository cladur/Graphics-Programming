#include "mesh.h"

int TextureTypeToTextureUnit(std::string type);

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    setupMesh();
}

void Mesh::Draw(Shader& shader, TexturePackingCombination texture_packing_combination)
{
    for (unsigned int i = 0; i < textures.size(); i++) {
        if (texture_packing_combination == TexturePackingCombination::AO_METALLIC_ROUGHNESS && (textures[i].type == "roughness_map" || textures[i].type == "ao_map")) {
            shader.setBool("has_ao_map", true);
            continue;
        }
        if (texture_packing_combination == TexturePackingCombination::METALLIC_ROUGHNESS && textures[i].type == "roughness_map") {
            continue;
        }
        if (textures[i].type == "emission_map") {
            shader.setBool("has_emission_map", true);
        }
        if (textures[i].type == "ao_map") {
            shader.setBool("has_ao_map", true);
        }
        int textureUnit = TextureTypeToTextureUnit(textures[i].type);
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        shader.setInt(textures[i].type, textureUnit);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }
    glActiveTexture(GL_TEXTURE0);

    // draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // kind of a hack
    shader.setBool("has_emission_map", false);
    shader.setBool("has_ao_map", false);
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
        &indices[0], GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);
}

int TextureTypeToTextureUnit(std::string type)
{
    if (type == "albedo_map")
        return 3;
    else if (type == "normal_map")
        return 4;
    else if (type == "metallic_map")
        return 5;
    else if (type == "roughness_map")
        return 6;
    else if (type == "ao_map")
        return 7;
    else if (type == "emission_map")
        return 8;

    std::cout << "Unknown texture type: " << type << std::endl;
    assert(false);
    return 0;
}
