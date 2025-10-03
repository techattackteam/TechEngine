#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textCoord;


out VS_OUT {
    vec4 fragPos;
} vs_out;

struct ObjectData {
    mat4 modelMatrix;
    uint materialIndex;
};

layout (std430, binding = 1) buffer ObjectBuffer {
    ObjectData objects[];
};


void main() {
    int objectIndex = gl_BaseInstance + gl_InstanceID;

    mat4 model = objects[objectIndex].modelMatrix;
    vs_out.fragPos = model * vec4(position, 1.0f);
}