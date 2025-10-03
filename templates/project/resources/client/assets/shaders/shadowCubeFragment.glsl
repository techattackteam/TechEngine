#version 460 core

in VS_OUT {
    vec4 fragPos; // World position from geometry shader
} fs_in;

// These must be passed from your C++ application
uniform float u_farPlane;

void main()
{
    // 1. Calculate the linear distance from the light to the fragment.
    float lightDistance = length(fs_in.fragPos.xyz - vec3(0.0, 4.0, 0.0));

    // 2. Normalize this distance to the [0, 1] range using the far plane.
    lightDistance = lightDistance / u_farPlane;

    // 3. Write this linear, normalized value to the depth buffer.
    gl_FragDepth = lightDistance;
}