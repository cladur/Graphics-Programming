#include "entity.h"

glm::mat4 Transform::getLocalModelMatrix()
{
    isDirty = false;

    // translation * rotation * scale (also know as TRS matrix)
    return glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(orient) * glm::scale(glm::mat4(1.0f), scale);
}

void Transform::computeModelMatrix()
{
    modelMatrix = getLocalModelMatrix();
}

void Transform::computeModelMatrix(const glm::mat4& parentGlobalModelMatrix)
{
    modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
}

void Entity::updateSelfAndChildren()
{
    if (transform.isDirty) {
        if (parent)
            transform.computeModelMatrix(parent->transform.modelMatrix);
        else
            transform.computeModelMatrix();

        for (auto&& child : children) {
            child->transform.isDirty = true;
            child->updateSelfAndChildren();
        }
    } else {
        for (auto&& child : children) {
            child->updateSelfAndChildren();
        }
    }
}

void Entity::forceUpdateSelfAndChildren()
{
    if (parent)
        transform.computeModelMatrix(parent->transform.modelMatrix);
    else
        transform.computeModelMatrix();

    for (auto&& child : children) {
        child->forceUpdateSelfAndChildren();
    }
}
