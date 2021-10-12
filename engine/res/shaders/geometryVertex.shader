#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textCoord;
layout (location = 3) in vec4 color;

out vec4 vertexColor;
out vec3 vertexNormal;
out vec3 fragPos;
out vec4 fragPosLightSpace;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main(){
    fragPos = vec3(model * vec4(position, 1.0f));
    vertexColor = color;
    vertexNormal = mat3(transpose(inverse(model))) * normal;
    fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0f);
    gl_Position = projection * view * model * vec4(position, 1.0f);
}