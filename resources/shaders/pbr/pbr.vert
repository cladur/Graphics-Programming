#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

#define MAX_SHADOWS 10

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;
out vec4 WorldPosLightSpaces[MAX_SHADOWS];

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform mat4 lightSpaceMatrices[MAX_SHADOWS];

void main()
{
    TexCoords = aTexCoords;
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(model) * aNormal;
    for (int i = 0; i < MAX_SHADOWS; ++i) {
        WorldPosLightSpaces[i] = lightSpaceMatrices[i] * vec4(WorldPos, 1.0);
    }

    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
