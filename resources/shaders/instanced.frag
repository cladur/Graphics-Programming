#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main()
{
    vec2 texCoords = TexCoords;
    texCoords.y = 1.0 - texCoords.y;
    FragColor = texture(texture_diffuse1, texCoords);
}
