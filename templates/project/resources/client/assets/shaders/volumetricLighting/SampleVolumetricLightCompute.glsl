#version 460 core

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0, rgba16f) uniform image2D u_hdrBuffer;
layout (binding = 1) uniform sampler3D u_volumetricLightingVolume;
layout (binding = 2) uniform sampler2D u_depthBuffer;

uniform mat4 u_viewProjection;
uniform float u_blendingFactor;

vec3 reconstructWorldPosition(vec2 uv, float depth) {
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePosition = inverse(u_viewProjection) * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    return viewSpacePosition.xyz;
}

vec4 sampleVolumetricLight(vec3 worldPos) {
    vec4 p = u_viewProjection * vec4(worldPos, 1.0f);

    if (p.w > 0.0f)
    {
        p.x /= p.w;
        p.y /= p.w;
        p.z /= p.w;
    }

    vec3 uvw;

    float z_buffer_params_y = 500 / 0.1f;
    float z_buffer_params_x = 1.0f - z_buffer_params_y;

    uvw.x = p.x * 0.5f + 0.5f;
    uvw.y = p.y * 0.5f + 0.5f;
    uvw.z = 1.0f / (z_buffer_params_x * (p.z * 0.5f + 0.5f) + z_buffer_params_y);

    // Exponential View-Z
    vec2 params = vec2(float(128) / log2(500 / 0.1f), -(float(128) * log2(0.1f) / log2(500 / 0.1f)));

    float view_z = uvw.z * 500;
    uvw.z = (max(log2(view_z) * params.x + params.y, 0.0f)) / 128;

    return textureLod(u_volumetricLightingVolume, uvw, 0);
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(u_hdrBuffer);
    if (pixelCoords.x >= imageSize.x || pixelCoords.y >= imageSize.y) {
        return;
    }

    vec2 uv = (vec2(pixelCoords) + vec2(0.5)) / vec2(imageSize);
    float depth = texture(u_depthBuffer, uv).r;
    vec3 worldPosition = reconstructWorldPosition(uv, depth);

    vec4 volumetricLight = sampleVolumetricLight(worldPosition);
    vec4 hdrColor = imageLoad(u_hdrBuffer, pixelCoords);
    vec4 color = hdrColor * volumetricLight.a + vec4(volumetricLight.rgb, 1.0f);
    hdrColor = mix(hdrColor, color, u_blendingFactor);
    imageStore(u_hdrBuffer, pixelCoords, hdrColor);
}