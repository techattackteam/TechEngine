#version 460 core
#extension GL_ARB_bindless_texture: enable
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0, rgba16f) uniform image2D u_volumetricOutput;

layout (binding = 0) uniform sampler2D u_depthBuffer;

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

layout (std430, binding = 3) readonly buffer LightBuffer {
    Light lights[];
};

uniform mat4 u_invViewProjection;
uniform vec3 u_cameraPosition;
uniform int u_lightIndex;

// Volumetric parameters
uniform int u_numSteps = 64;
uniform float u_scatteringCoeff = 0.05;
uniform float u_maxDistance = 100.0;
uniform float u_ditherScale = 1.0;

// Blue noise for temporal dithering (optional)
layout (binding = 2) uniform sampler2D u_blueNoise;
uniform float u_frameIndex;

vec3 reconstructWorldPosition(vec2 uv, float depth) {
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos = u_invViewProjection * ndc;
    return worldPos.xyz / worldPos.w;
}

// Sample shadow map for a world position
float sampleShadowMap(vec3 worldPos, Light light) {
    if (light.castShadow == 0) return 1.0;

    mat4 lightMatrix = light.lightSpaceMatrix[0];
    vec4 lightSpacePos = lightMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Out of shadow map bounds = not in shadow
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
    projCoords.y < 0.0 || projCoords.y > 1.0 ||
    projCoords.z > 1.0) {
        return 1.0;
    }

    sampler2D shadowHandle = sampler2D(light.shadowHandle[0]);
    float closestDepth = texture(shadowHandle, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = 0.005;
    return (currentDepth - bias < closestDepth) ? 1.0 : 0.0;
}

// Interleaved gradient noise for dithering
float interleavedGradientNoise(vec2 coord) {
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(coord, magic.xy)));
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(u_volumetricOutput);

    if (pixelCoord.x >= imageSize.x || pixelCoord.y >= imageSize.y) {
        return;
    }

    vec2 uv = (vec2(pixelCoord) + 0.5) / vec2(imageSize);
    float sceneDepth = texture(u_depthBuffer, uv).r;

    Light light = lights[u_lightIndex];

    // Reconstruct ray
    vec3 rayStart = u_cameraPosition;
    vec3 rayEnd = reconstructWorldPosition(uv, sceneDepth);
    vec3 rayDir = rayEnd - rayStart;
    float rayLength = length(rayDir);
    rayDir = normalize(rayDir);

    // Clamp march distance
    float marchDistance = min(rayLength, u_maxDistance);

    // Apply dithering to starting position to reduce banding
    float dither = interleavedGradientNoise(vec2(pixelCoord) + u_frameIndex);
    dither = dither * 2.0 - 1.0; // [-1, 1]

    float stepSize = marchDistance / float(u_numSteps);
    float t = stepSize * dither * u_ditherScale;

    vec3 scattering = vec3(0.0);

    // Raymarch through volume
    for (int i = 0; i < u_numSteps; i++) {
        vec3 samplePos = rayStart + rayDir * t;

        float visibility = 1.0;
        float attenuation = 1.0;

        if (light.type == 1) {
            // Directional
            // Check if this point in space can see the light
            visibility = sampleShadowMap(samplePos, light);
        }
        else if (light.type == 0) {
            // Point light
            vec3 toLight = light.position - samplePos;
            float distToLight = length(toLight);

            if (distToLight < light.radius) {
                // Simple attenuation
                attenuation = 1.0 - pow(distToLight / light.radius, 2.0);
                attenuation = max(attenuation, 0.0);

                // Check shadows (would need cube shadow map)
                // visibility = sampleCubeShadowMap(samplePos, light);
            } else {
                attenuation = 0.0;
            }
        }
        else if (light.type == 2) {
            // Spot light
            vec3 toLight = light.position - samplePos;
            float distToLight = length(toLight);
            vec3 lightDir = normalize(toLight);

            float theta = dot(lightDir, normalize(-light.direction));
            float epsilon = light.innerCutoff - light.outerCutoff;
            float spotEffect = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

            if (distToLight < light.radius && spotEffect > 0.0) {
                attenuation = (1.0 - pow(distToLight / light.radius, 2.0)) * spotEffect;
                visibility = sampleShadowMap(samplePos, light);
            } else {
                attenuation = 0.0;
            }
        }

        // Accumulate scattering
        float scatteringAmount = u_scatteringCoeff * stepSize * visibility * attenuation;
        scattering += light.color * light.intensity * scatteringAmount;

        t += stepSize;
    }

    imageStore(u_volumetricOutput, pixelCoord, vec4(scattering, 1.0));
}