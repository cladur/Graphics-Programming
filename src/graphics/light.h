#ifndef LIGHT_H
#define LIGHT_H

#include "entity.h"

enum class LightType {
    DIRECTIONAL,
    POINT,
    SPOT
};

class Light : public Entity {
public:
    LightType type;
    glm::vec3 color = { 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
    float innerAngle = 30.0f;
    float outerAngle = 45.0f;
    float spotAngle = 30.0f;

    Light(std::string name, LightType type)
        : Entity(name)
        , type(type)
    {
    }
    void Draw(Shader& shader)
    {
    }
};

#endif
