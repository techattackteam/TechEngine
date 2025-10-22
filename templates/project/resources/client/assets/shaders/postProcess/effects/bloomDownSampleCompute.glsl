#version 460 core

#extension GL_EXT_shader_image_load_formatted: require

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (binding = 0) uniform readonly image2D u_sourceMip;

layout (binding = 1, rgba16f) uniform writeonly image2D u_destinationMip;

uniform ivec2 u_sourceResolution;

#define KERNEL_RADIUS 5

const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.0540540, 0.0162162);

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dimension = imageSize(u_destinationMip);

    if (pixelCoords.x >= dimension.x || pixelCoords.y >= dimension.y) {
        return;
    }

    ivec2 uv = ivec2(pixelCoords) * 2;

    ivec2 srcTexelSize = 1 / u_sourceResolution;

    vec3 result = imageLoad(u_sourceMip, pixelCoords).rgb * weights[0];

    vec3 a = imageLoad(u_sourceMip, uv + srcTexelSize * ivec2(-2, 2)).rgb;
    vec3 b = imageLoad(u_sourceMip, uv + srcTexelSize * ivec2(0, 2)).rgb;
    vec3 c = imageLoad(u_sourceMip, uv + srcTexelSize * ivec2(2, 2)).rgb;

    vec3 d = imageLoad(u_sourceMip, uv + srcTexelSize * ivec2(-2, 0)).rgb;
    vec3 e = imageLoad(u_sourceMip, uv + srcTexelSize * ivec2(0, 0)).rgb;
    vec3 f = imageLoad(u_sourceMip, uv + srcTexelSize * ivec2(2, 0)).rgb;

    vec3 g = imageLoad(u_sourceMip, uv + srcTexelSize * ivec2(-2, -2)).rgb;
    vec3 h = imageLoad(u_sourceMip, uv + srcTexelSize * ivec2(0, -2)).rgb;
    vec3 i = imageLoad(u_sourceMip, uv + srcTexelSize * ivec2(2, -2)).rgb;

    vec3 j = imageLoad(u_sourceMip, uv + srcTexelSize).rgb;
    vec3 k = imageLoad(u_sourceMip, uv + srcTexelSize).rgb;
    vec3 l = imageLoad(u_sourceMip, uv + srcTexelSize).rgb;
    vec3 m = imageLoad(u_sourceMip, uv + srcTexelSize).rgb;

    vec3 downsample = e * 0.125;
    downsample += (a + c + g + i) * 0.03125;
    downsample += (b + d + f + h) * 0.0625;
    downsample += (j + k + l + m) * 0.125;

    imageStore(u_destinationMip, pixelCoords, vec4(downsample, 1.0));
}