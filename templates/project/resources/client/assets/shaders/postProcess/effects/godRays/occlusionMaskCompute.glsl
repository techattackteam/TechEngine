#version 460 core

#extension GL_ARB_bindless_texture: enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0) uniform sampler2D u_hdrBuffer;
layout (binding = 1) uniform sampler2D u_depthBuffer;
layout (binding = 2) uniform sampler2D u_normalBuffer;

layout (binding = 0, rgba16f) uniform writeonly image2D u_occlusionMask;

struct Light {
    vec3 position;
    int type; // 0 = point, 1 = directional, 2 = spot

    vec3 direction;
    float radius;

    vec3 color;
    float intensity;

    float innerCutoff;
    float outerCutoff;
    int castShadow;
    float pad2;

    uvec2 shadowHandle[4];

    mat4 lightSpaceMatrix[4];

    float cascadeSplits[4];
};

uniform int u_lightIndex;
uniform mat4 u_invViewProjectionMatrix;
uniform float u_brightThreshold = 1.0;

layout (std430, binding = 3) readonly buffer LightBuffer {
    Light lights[];
};
vec3 reconstructWorldPosition(vec2 uv, float depth) {
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos = u_invViewProjectionMatrix * ndc;
    return worldPos.xyz / worldPos.w;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(u_occlusionMask);

    if (pixelCoord.x >= imageSize.x || pixelCoord.y >= imageSize.y) {
        return;
    }

    vec2 uv = (vec2(pixelCoord) + 0.5) / vec2(imageSize);

    float depth = texture(u_depthBuffer, uv).r;
    if (depth >= 0.9999) {
        imageStore(u_occlusionMask, pixelCoord, vec4(0.0, 0.0, 0.0, 1.0));
        return;
    }
    float mask = 1.0;
/**
    vec3 hdr = texture(u_hdrBuffer, uv).rgb;
    float lum = dot(hdr, vec3(0.2126, 0.7152, 0.0722));
    mask = (lum >= u_brightThreshold) ? 1.0 : 0.0;

    if (mask <= 0.0) {
        imageStore(u_occlusionMask, pixelCoord, vec4(0.0, 0.0, 0.0, 1.0));
        return;
    }*/
    Light light = lights[u_lightIndex];
    if (light.castShadow == 0) {
        imageStore(u_occlusionMask, pixelCoord, vec4(mask, 0.0, 0.0, 1.0));
        return;
    }

    vec3 worldPosition = reconstructWorldPosition(uv, depth);

    float distance;
    vec3 normal = texture(u_normalBuffer, uv).xyz * 2.0 - 1.0;

    if (light.type == 1) {
        // Check if surface faces the light
        float facing = dot(normal, -light.direction);

        if (facing <= 0.0) {
            // Back-facing surfaces definitely occluded
            imageStore(u_occlusionMask, pixelCoord, vec4(0.0, 0.0, 0.0, 1.0));
            return;
        }

        mat4 lightMatrix = light.lightSpaceMatrix[0];
        vec4 fragToLight = lightMatrix * vec4(worldPosition, 1.0);
        sampler2D handle = sampler2D(light.shadowHandle[0]);

        vec3 coords = fragToLight.xyz / fragToLight.w;
        coords = coords * 0.5 + 0.5;

        float closestDepth = texture(handle, coords.xy).r;
        float currentDepth = coords.z;
        float bias = max(0.05 * (1.0 - facing), 0.0005);

        // In shadow = occluded (black)
        // Lit = can see sun (white * facing factor)
        float shadowFactor = (currentDepth - bias > closestDepth) ? 0.0 : 1.0;

        // Modulate by how much surface faces the light
        float occlusion = shadowFactor * smoothstep(0.0, 0.3, facing);

        imageStore(u_occlusionMask, pixelCoord, vec4(occlusion, 0.0, 0.0, 1.0));
        return;
    }
    //mask *= shadowFactor;



/** else if (light.type == 2) {
        // === SPOTLIGHT ===
        vec4 lp = light.lightSpaceMatrix[0] * vec4(worldPosition, 1.0);
        vec3 ndc = lp.xyz / lp.w;
        vec2 shadowUV = ndc.xy * 0.5 + 0.5;
        float lightDepth = ndc.z * 0.5 + 0.5;

        if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0) {
            shadowFactor = 1.0;
        } else {
            sampler2D shadowSampler = sampler2D(light.shadowHandle[0]);
            float d = texture(shadowSampler, shadowUV).r;
            shadowFactor = (lightDepth *//**+ u_shadowBias *//** <= d) ? 1.0 : 0.0;
        *//**            if (u_pcf == 1) {
                ivec2 s = textureSize(u_shadowMaps2D[idx], 0);
                vec2 texel = 1.0 / vec2(s);
                shadowFactor = pcf2D(u_shadowMaps2D[idx], shadowUV, lightDepth + u_shadowBias, texel);
            } else {

            }*//**
        }
    } else if (light.type == 0) {
        // === POINT LIGHT ===
        vec3 fragToLight = worldPosition - light.position;
        float dist = length(fragToLight);

        // Early exit if outside radius - store 0 with large distance
        if (dist > light.radius) {
            imageStore(u_occlusionMask, pixelCoord, vec4(0.0, 0.0, 0.0, 1.0));
            return;
        }

        vec3 direction = fragToLight / max(dist, 1e-6);

        samplerCube shadowCube = samplerCube(light.shadowHandle[0]);
        float closestDepth = texture(shadowCube, direction).r;
        shadowFactor = (dist > closestDepth + 0.001) ? 1.0 : 0.0;

        // Combine brightness mask with shadow
        float occlusion = mask * shadowFactor;

        // Store occlusion in R, distance in G
        imageStore(u_occlusionMask, pixelCoord, vec4(occlusion, dist, 0.0, 1.0));
        return;

    }*/
}