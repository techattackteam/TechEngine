#version 460

layout (location = 0) in vec3 position;

out vec3 v_localPos;

uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    v_localPos = position;
    gl_Position = u_projection * u_view * vec4(v_localPos, 1.0);
}