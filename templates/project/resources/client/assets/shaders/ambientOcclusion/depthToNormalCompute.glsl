#version 460
layout (local_size_x = 8, local_size_y = 8) in;

layout (binding = 0, rgba16f) uniform image2D u_outNormal; // output view-space normals

uniform sampler2D u_depthTexture;
uniform mat4 u_inverseProjection;
uniform ivec2 u_screenSize;

vec3 reconstructViewPos(vec2 uv, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 view = u_inverseProjection * clip;
    return view.xyz / view.w;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoord.x >= u_screenSize.x || pixelCoord.y >= u_screenSize.y) {
        return;
    }
    vec2 uv = (vec2(pixelCoord) + 0.5) / vec2(u_screenSize);

    vec2 texel = 1.0 / vec2(u_screenSize);

    // sample depths around pixel
    float dzdx = 0.0;
    float dzdy = 0.0;

    // 3x3 Sobel kernel on view-space positions
    mat3 kx = mat3(-1, 0, 1,
    -2, 0, 2,
    -1, 0, 1);

    mat3 ky = mat3(-1, -2, -1,
    0, 0, 0,
    1, 2, 1);

    vec3 gx = vec3(0.0);
    vec3 gy = vec3(0.0);

    for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
            vec2 offset = vec2(i, j) * texel;
            float d = texture(u_depthTexture, uv + offset).r;
            vec3 p = reconstructViewPos(uv + offset, d);
            gx += kx[j + 1][i + 1] * p;
            gy += ky[j + 1][i + 1] * p;
        }
    }

    vec3 n = cross(gy, gx);

    if (dot(n, n) < 0.0001) {
        imageStore(u_outNormal, pixelCoord, vec4(0.0, 0.0, -1.0, 1.0));
        return;
    }

    n = normalize(n);

    if (n.z > 0.0) {
        n = -n;
    }
    imageStore(u_outNormal, pixelCoord, vec4(n, 1.0));
}
