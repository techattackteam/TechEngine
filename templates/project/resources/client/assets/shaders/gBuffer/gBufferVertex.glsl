#version 460 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textCoord;

uniform mat4 u_view;
uniform mat4 u_projection;

struct ObjectData {
    mat4 modelMatrix;
    uint materialIndex;
};

layout (std430, binding = 1) buffer ObjectBuffer {
    ObjectData objects[];
};

out vec3 v_wordPos;
out vec3 v_viewPos;
out vec3 v_normal;
out vec2 v_textCoord;
out flat uint v_materialIndex;

void main() {
    int objectIndex = gl_BaseInstance + gl_InstanceID;

    mat4 model = objects[objectIndex].modelMatrix;

    vec4 worldPos = model * vec4(position, 1.0);
    vec4 viewPos = u_view * worldPos;

    v_wordPos = worldPos.xyz;
    v_viewPos = viewPos.xyz;

    v_normal = mat3(transpose(inverse(model))) * normal;
    v_textCoord = textCoord;
    v_materialIndex = objects[objectIndex].materialIndex;

    gl_Position = u_projection * viewPos;
}