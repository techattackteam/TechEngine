#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 textCoord;

out vec3 v_localPos;
out vec2 v_texCoord;

void main() {
    v_localPos = position;
    v_texCoord = textCoord;
    gl_Position = vec4(position, 1.0);
}