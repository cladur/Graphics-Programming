#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

#include <list>
#include <memory>
#include <vector>

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

struct Transform {
    /*SPACE INFORMATION*/
    // Local space information
    glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
    glm::quat orient = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

    // Global space information concatenate in matrix
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    bool isDirty = true;

    glm::mat4 getLocalModelMatrix();
    void computeModelMatrix();
    void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix);
};

class Entity {
public:
    std::string name;
    Transform transform;
    std::vector<std::unique_ptr<Entity>> children;
    Entity* parent = nullptr;

    // constructor, expects a filepath to a 3D model.
    Entity(std::string name)
        : name(name)
    {
    }

    void addChild(std::unique_ptr<Entity> entity)
    {
        children.emplace_back(std::move(entity));
        children.back()->parent = this;
    }

    void updateSelfAndChildren();
    void forceUpdateSelfAndChildren();

    virtual void Draw(Shader& shader)
    {
    }
};

#endif
