#version 460 core

out vec4 fragColor;

in vec3 fragPos;
in vec4 vertexColor;
in vec3 vertexNormal;
in vec4 fragPosLightSpace;
in vec2 fragTextCoord;


uniform sampler2D diffuseTexture;

void main() {
    fragColor = vertexColor;
}