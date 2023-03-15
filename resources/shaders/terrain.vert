#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;

out VS_OUT
{
    vec2 texCoords;
}
vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform sampler2D heightMap;

void main()
{
    float height = texture(heightMap, aTexCoords).r;
    gl_Position = vec4(aPos.x, height * 0.2f, aPos.z, 1.0f);
    vs_out.texCoords = aTexCoords;
    // out_normal = mat3(transpose(inverse(model))) * normal;
}
