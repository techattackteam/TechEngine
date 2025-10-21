#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textCoord;

uniform mat4 u_projection;
uniform mat4 u_view;

out vec3 v_localPos;
out vec2 v_texCoord;
void main() {
    v_localPos = position;

    mat4 rotView = mat4(mat3(u_view));
    vec4 clipPos = u_projection * rotView * vec4(v_localPos, 1.0);

    v_texCoord = textCoord;

    gl_Position = clipPos.xyww;
}