#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D heightMap;

void main()
{
    float value = texture(heightMap, TexCoords).r;
    FragColor = vec4(value, value, value, 1.0);
}
