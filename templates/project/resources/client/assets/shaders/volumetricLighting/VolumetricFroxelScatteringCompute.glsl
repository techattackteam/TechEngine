#version 460 core
#extension GL_NV_uniform_buffer_std430_layout: enable
#extension GL_ARB_bindless_texture: enable

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
    vec3 scatteringCoefficient;
    float density;

    vec3 absorptionCoefficient;
    float anisotropy;

    vec3 emissiveCoefficient;
    bool enabled;
};

layout (std430, binding = 0) readonly buffer LightBuffer {
    Light lights[];
};

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
    float view_z = froxelNearPlane * pow(froxelFarPlane / froxelNearPlane, (float(froxelCoord.z) + 0.5f) / float(froxelDimensions.z));

    vec3 uv = vec3((float(froxelCoord.x) + 0.5f) / float(froxelDimensions.x),
    (float(froxelCoord.y) + 0.5f) / float(froxelDimensions.y),
    view_z / froxelFarPlane);
    vec3 ndc;
    float z_buffer_params_y = froxelFarPlane / froxelNearPlane;
    float z_buffer_params_x = 1.0f - z_buffer_params_y;
    float linear = (1.0f / uv.z - z_buffer_params_y) / z_buffer_params_x;
    ndc.x = 2.0f * uv.x - 1.0f;
    ndc.y = 2.0f * uv.y - 1.0f;
    ndc.z = 2.0f * linear - 1.0f;

    vec4 p = viewProjectionInverse * vec4(ndc, 1.0f);

    p.x /= p.w;
    p.y /= p.w;
    p.z /= p.w;

    return p.xyz;
}

// Henyey-Greenstein phase function
float phaseHG(float cosTheta, float g) {
    const float PI = 3.14159265359;
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0f / (4.0f * PI)) * (1.0f - g2) / max(pow(denom, 1.5f), 0.0001f);
}

float sampleDensity(vec3 worldPos) {
    //float heightDensity = globalDensity.y /*** exp(-worldPos.y * heightFalloff)*/;

    // For now, just return global density
    // Later we'll add local volume sampling here

    //return heightDensity;
    return 1.0f;
}

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

    return light.color * light.intensity * attenuation * phase;
}

float calculateDirectionalShadow(Light light, vec3 worldPos, vec4 viewPos) {
    float depthFromCamera = abs(viewPos.z / viewPos.w);

    int cascadeIndex = 0;
    for (int i = 0; i < 4; i++) {
        if (depthFromCamera > light.cascadeSplits[i]) {
            cascadeIndex = i + 1;
        }
    }
    cascadeIndex = 0;

    mat4 lightMatrix = light.lightSpaceMatrix[cascadeIndex];
    vec4 fragPosLightSpace = lightMatrix * vec4(worldPos, 1.0);

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float currentDepth = projCoords.z;

    float bias = 0.005;
    sampler2D handle = sampler2D(light.shadowHandle[cascadeIndex]);
    float closestDepth = texture(handle, projCoords.xy).r;
    float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
    return 1.0 - shadow;
}

vec3 evaluateDirectionalLight(Light light, vec3 worldPos, vec4 viewPos, vec3 viewDir, float density) {
    float shadow = 1.0;
    if (light.castShadow != 0) {
        // Pass the world and view position to the shadow function
        shadow = calculateDirectionalShadow(light, worldPos, viewPos);
    }

    if (shadow <= 0.0) {
        return vec3(0.0);
    }

    // Phase function
    float cosTheta = dot(viewDir, light.direction);
    float phase = phaseHG(cosTheta, anisotropy);

    // Light contribution
    return light.color * light.intensity * phase * shadow;
}
void main() {
    uvec3 froxelCoords = gl_GlobalInvocationID;

    if (froxelCoords.x >= froxelDimensions.x || froxelCoords.y >= froxelDimensions.y || froxelCoords.z >= froxelDimensions.z) {
        return;
    }

    vec3 worldPosition = froxelToWorldPos(froxelCoords);
    vec4 viewPosition = viewMatrix * vec4(worldPosition, 1.0);
    vec3 viewDirection = normalize(cameraPosition - worldPosition);

    vec3 sigma_s = scatteringCoefficient;
    vec3 sigma_a = absorptionCoefficient;

    vec3 extinctionCoefficient = sigma_s + sigma_a;
    vec3 scatteringAlbedo = sigma_s / max(extinctionCoefficient, vec3(0.00001)); // max() to avoid divide by zero

    vec3 inScattering = vec3(0.0f);

    for (uint i = 0u; i < lights.length(); ++i) {
        Light light = lights[i];

        if (light.type == 0) {
            // Point light
            inScattering += evaluatePointLight(light, worldPosition, viewDirection, density);
        } else if (light.type == 1) {
            // Directional light
            inScattering += evaluateDirectionalLight(light, worldPosition, viewPosition, viewDirection, density);
        }
    }

    inScattering += vec3(0.0f); // Ambient term
    inScattering += emissiveCoefficient;

    vec3 finalColor = inScattering * sigma_s;

    float froxelDepth = froxelZToDepth(float(froxelCoords.z + 1)) - froxelZToDepth(float(froxelCoords.z));
    //vec3 transmittance = exp(-extinctionCoefficient * froxelDepth);
    //float avgTransmittance = (transmittance.r + transmittance.g + transmittance.b) / 3.0;
    float avgExtinction = (extinctionCoefficient.r + extinctionCoefficient.g + extinctionCoefficient.b) / 3.0;
    imageStore(froxelScattering, ivec3(froxelCoords), vec4(finalColor, avgExtinction));
}