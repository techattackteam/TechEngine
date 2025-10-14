#version 460

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (binding = 0) uniform sampler2D u_lowerMip;
layout (binding = 1, rgba16f) uniform image2D u_higherMip;

uniform float u_intensity = 1.0;
uniform int u_lowerMipLevel;

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 higherMipSize = imageSize(u_higherMip);

    if (pixelCoords.x >= higherMipSize.x || pixelCoords.y >= higherMipSize.y) {
        return;
    }

    vec2 uv = (vec2(pixelCoords) + 0.5) / vec2(higherMipSize);

    vec2 lowerMipSize = vec2(textureSize(u_lowerMip, u_lowerMipLevel));
    vec2 texelSize = 1.0 / lowerMipSize;

    vec3 a = textureLod(u_lowerMip, uv + vec2(-texelSize.x, texelSize.y), u_lowerMipLevel).rgb;
    vec3 b = textureLod(u_lowerMip, uv + vec2(0.0, texelSize.y), u_lowerMipLevel).rgb;
    vec3 c = textureLod(u_lowerMip, uv + vec2(texelSize.x, texelSize.y), u_lowerMipLevel).rgb;

    vec3 d = textureLod(u_lowerMip, uv + vec2(-texelSize.x, 0.0), u_lowerMipLevel).rgb;
    vec3 e = textureLod(u_lowerMip, uv + vec2(0.0, 0.0), u_lowerMipLevel).rgb;
    vec3 f = textureLod(u_lowerMip, uv + vec2(texelSize.x, 0.0), u_lowerMipLevel).rgb;

    vec3 g = textureLod(u_lowerMip, uv + vec2(-texelSize.x, -texelSize.y), u_lowerMipLevel).rgb;
    vec3 h = textureLod(u_lowerMip, uv + vec2(0.0, -texelSize.y), u_lowerMipLevel).rgb;
    vec3 i = textureLod(u_lowerMip, uv + vec2(texelSize.x, -texelSize.y), u_lowerMipLevel).rgb;

    vec3 upsample = e * 4.0;
    upsample += (b + d + f + h) * 2.0;
    upsample += (a + c + g + i);
    upsample *= 1.0 / 16.0;

    vec3 existingColor = imageLoad(u_higherMip, pixelCoords).rgb;
    vec3 result = existingColor + upsample * u_intensity;

    imageStore(u_higherMip, pixelCoords, vec4(result, 1.0));
}