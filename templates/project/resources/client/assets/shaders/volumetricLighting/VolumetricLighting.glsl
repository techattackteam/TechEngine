#version 460 core
#extension GL_NV_uniform_buffer_std430_layout: enable

struct Light {
    vec3 position;
    int type;

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

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0, rgba16f) uniform image3D froxelScattering;

layout (std430, binding = 0) uniform FroxelParams {
    mat4 viewProjectionInverse;
    mat4 viewMatrix;

    uvec3 froxelDimensions;
    float froxelNearPlane;

    float froxelFarPlane;
    float depthDistributionScale;
    uint useExponentialDepth;
    float rcpFroxelDimX;

    float rcpFroxelDimY;
    float rcpFroxelDimZ;
    vec2 padding_0;

    vec3 cameraPosition;
    float padding_1;
};

layout (std430, binding = 1) uniform VolumetricSettings {
    vec3 globalDensity;
    float heightFalloff;

    vec3 globalAlbedo;
    float anisotropy;

    float globalExtinction;
    float ambientIntensity;
    //vec2 padding;
};

layout (std430, binding = 0) readonly buffer LightBuffer {
    Light lights[];
};

float depthToFroxelZ(float linearDepth) {
    if (useExponentialDepth != 0u) {
        float depthRange = froxelFarPlane - froxelNearPlane;
        float normalizedDepth = (linearDepth - froxelNearPlane) / depthRange;

        float scale = depthDistributionScale;
        float exponentialDepth = (exp(normalizedDepth * scale) - 1.0) / (exp(scale) - 1.0);

        return exponentialDepth * float(froxelDimensions.z);
    } else {
        float normalizedDepth = (linearDepth - froxelNearPlane) / (froxelFarPlane - froxelNearPlane);
        return normalizedDepth * float(froxelDimensions.z);
    }
}

float froxelZToDepth(float froxelZ) {
    float normalizedZ = froxelZ / float(froxelDimensions.z);

    if (useExponentialDepth != 0u) {
        float scale = depthDistributionScale;
        float linearZ = log(normalizedZ * (exp(scale) - 1.0) + 1.0) / scale;
        return froxelNearPlane + linearZ * (froxelFarPlane - froxelNearPlane);
    } else {
        return froxelNearPlane + normalizedZ * (froxelFarPlane - froxelNearPlane);
    }
}

vec3 froxelToWorldPos(uvec3 froxelCoord) {
    // Convert to UV coordinates [0, 1]
    vec2 uv = (vec2(froxelCoord.xy) + 0.5) / vec2(froxelDimensions.xy);

    // Get view-space depth
    float viewDepth = froxelZToDepth(float(froxelCoord.z) + 0.5);

    // Create NDC position
    vec2 ndc;
    ndc.x = uv.x * 2.0 - 1.0;
    ndc.y = (1.0 - uv.y) * 2.0 - 1.0;

    // Reconstruct view-space position
    // Transform NDC ray to view space
    vec4 clipPos = vec4(ndc, 0.0, 1.0);
    vec4 viewPos = viewProjectionInverse * clipPos;
    viewPos.xyz /= viewPos.w;

    // This gives us the direction from camera
    vec3 viewDir = normalize(viewPos.xyz - cameraPosition);

    // Scale by depth (this is an approximation for perspective)
    // For accurate results, we need to account for perspective divide
    vec3 worldPos = cameraPosition + viewDir * viewDepth;

    return worldPos;
}

uvec3 worldPosToFroxel(vec3 worldPos) {
    vec4 viewPos = viewMatrix * vec4(worldPos, 1.0);
    float linearDepth = -viewPos.z; // View space Z

    vec4 clipPos = viewProjectionInverse * vec4(worldPos, 1.0);
    clipPos.xyz /= clipPos.w;

    vec2 uv = clipPos.xy * 0.5 + 0.5;
    uv.y = 1.0 - uv.y;

    uvec3 froxel;
    froxel.x = uint(uv.x * float(froxelDimensions.x));
    froxel.y = uint(uv.y * float(froxelDimensions.y));
    froxel.z = uint(depthToFroxelZ(linearDepth));

    froxel = clamp(froxel, uvec3(0), froxelDimensions - 1u);

    return froxel;
}

uvec3 screenToFroxel(vec2 screenUV, float linearDepth) {
    uvec3 froxel;
    froxel.x = uint(screenUV.x * float(froxelDimensions.x));
    froxel.y = uint(screenUV.y * float(froxelDimensions.y));
    froxel.z = uint(depthToFroxelZ(linearDepth));

    froxel = clamp(froxel, uvec3(0), froxelDimensions - 1u);
    return froxel;
}

// Henyey-Greenstein phase function
float phaseHG(float cosTheta, float g) {
    const float PI = 3.14159265359;
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0 - g2) / (4.0 * PI * denom * sqrt(denom));
}

// Sample density at world position
float sampleDensity(vec3 worldPos) {
    float heightDensity = globalDensity.y * exp(-worldPos.y * heightFalloff);

    // For now, just return global density
    // Later we'll add local volume sampling here
    return heightDensity;
}

// Calculate lighting contribution from a point light
vec3 evaluatePointLight(Light light, vec3 worldPos, vec3 viewDir, float density) {
    vec3 toLight = light.position - worldPos;
    float distToLight = length(toLight);
    vec3 lightDir = toLight / distToLight;

    // Distance attenuation
    float attenuation = 1.0 - smoothstep(light.radius * 0.8, light.radius, distToLight);
    attenuation = attenuation * attenuation;

    if (attenuation <= 0.0) {
        return vec3(0.0);
    }

    // Phase function
    float cosTheta = dot(lightDir, -viewDir);
    float phase = phaseHG(cosTheta, anisotropy);

    // Light contribution
    vec3 lighting = light.color * light.intensity * attenuation * phase;

    return lighting * density * globalAlbedo;
}

// Calculate lighting contribution from a directional light
vec3 evaluateDirectionalLight(Light light, vec3 worldPos, vec3 viewDir, float density) {
    vec3 lightDirection = -normalize(light.direction);
    return lightDirection;
    // Phase function
    float cosTheta = dot(lightDirection, -viewDir);
    float phase = phaseHG(cosTheta, anisotropy);

    // Light contribution
    vec3 lighting = light.color * light.intensity * phase;

    return lighting * density * globalAlbedo;
}
void main() {
    uvec3 froxelCoords = gl_GlobalInvocationID;

    // Bounds check
    if (any(greaterThanEqual(froxelCoords, froxelDimensions))) {
        return;
    }
/**    vec3 worldPosition = froxelToWorldPos(froxelCoords);
    Light light = lights[0];

    // Distance to light
    float distToLight = length(worldPosition - light.position);

    // Simple attenuation visualization
    float attenuation = 1.0 - smoothstep(0.0, light.radius, distToLight);

    // Color based on distance: close = bright, far = dark
    vec3 color = light.color * attenuation;

    imageStore(froxelScattering, ivec3(froxelCoords), vec4(color, 1.0));
    return;*/

    // Get world position of this froxel center
    vec3 worldPosition = froxelToWorldPos(froxelCoords);
    vec3 viewDirection = normalize((worldPosition - cameraPosition));
    float density = sampleDensity(worldPosition);

    if (density <= 0.0001) {
        imageStore(froxelScattering, ivec3(froxelCoords), vec4(0.0, 0.0, 0.0, 1.0));
        return;
    }

    vec3 totalScattering = vec3(0.0f);

    for (uint i = 0u; i < lights.length(); ++i) {
        Light light = lights[i];

        if (light.type == 0) {
            // Point light
            totalScattering += evaluatePointLight(light, worldPosition, viewDirection, density);
        } else if (light.type == 1) {
            // Directional light
            totalScattering += evaluateDirectionalLight(light, worldPosition, viewDirection, density);
        }
    }
    totalScattering += globalAlbedo * density * ambientIntensity;
    float extinction = globalExtinction * density;
    float transmittance = exp(-extinction);
    imageStore(froxelScattering, ivec3(froxelCoords), vec4(totalScattering, transmittance));
}