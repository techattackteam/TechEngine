#version 460

out vec4 fragColor;

in vec3 v_localPos;

uniform samplerCube u_envMap;

void main() {

    vec3 color = texture(u_envMap, v_localPos).rgb;

    fragColor = vec4(color, 1.0);

}