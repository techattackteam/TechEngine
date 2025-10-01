#version 460 core
#extension GL_ARB_shader_draw_parameters: enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textCoord;

out vec3 v_worldPos;
out vec3 v_normal;
out flat uint f_materialID;

uniform mat4 u_view;
uniform mat4 u_projection;

struct ObjectData {
    mat4 modelMatrix;
    uint materialID;
};

layout (std430, binding = 1) buffer ObjectBuffer {
    ObjectData objects[];
};


uniform bool isLightingActive;
void main() {
    int objectIndex = gl_BaseInstance + gl_InstanceID;;

    mat4 model = objects[objectIndex].modelMatrix;
    vec4 worldPos = model * vec4(position, 1.0f);
    v_worldPos = worldPos.xyz;

    v_normal = mat3(model) * normal;

    f_materialID = objects[objectIndex].materialID;

    gl_Position = u_projection * u_view * model * vec4(position, 1.0f);
}