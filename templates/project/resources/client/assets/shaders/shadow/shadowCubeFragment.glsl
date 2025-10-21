#version 460 core

in VS_OUT {
    vec4 fragPos;
} fs_in;

uniform float u_farPlane;
uniform vec3 u_lightPos;
void main()
{
    float lightDistance = length(fs_in.fragPos.xyz - u_lightPos);

    lightDistance = lightDistance / u_farPlane;

    gl_FragDepth = lightDistance;
}