#version 440 core

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoords;

uniform sampler2D uTexture;
uniform bool uUseTexture;

void main()
{
    if (uUseTexture) {
        FragColor = texture(uTexture, vTexCoords) * vColor;
    } else {
        FragColor = vColor;
    }
}