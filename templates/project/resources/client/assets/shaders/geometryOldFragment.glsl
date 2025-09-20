#version 460 core

out vec4 fragColor;

in vec3 fragPos;
in vec4 vertexColor;
in vec3 vertexNormal;
in vec4 fragPosLightSpace;
in vec2 fragTextCoord;


struct Material {
    bool useTexture;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform sampler2D diffuseTexture;

uniform Material material;

void main() {
    if (material.useTexture) {
        fragColor = texture(diffuseTexture, fragTextCoord);
    } else {
        fragColor = vertexColor;
    }
}