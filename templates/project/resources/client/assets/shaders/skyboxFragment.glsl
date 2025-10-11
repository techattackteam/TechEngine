#version 460

out vec4 fragColor;

in vec3 v_localPos;

uniform samplerCube u_envMap;

void main() {

    vec3 color = texture(u_envMap, normalize(v_localPos)).rgb;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    fragColor = vec4(color, 1.0);

}