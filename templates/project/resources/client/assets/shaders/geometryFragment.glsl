#version 460 core

out vec4 fragColor;

in vec3 v_worldPos;
in vec3 v_normal;
in flat uint f_materialID;

struct Material {
    vec4 albedo;
    float metallic;
    float roughness;
    float ao;
};

struct Light {
    vec3 position;
    vec3 color;
    float radius;
    float intensity;
};

struct TileInfo {
    uint offset; // Offset into the light index buffer
    uint lightsCount; // Number of lights affecting this tile
};

layout (std430, binding = 2) buffer MaterialBuffer {
    Material materials[];
} materialBuffer;

layout (std430, binding = 3) readonly buffer LightBuffer {
    Light lights[];
};

layout (std430, binding = 4) readonly buffer LightIndexBuffer {
    uint lightIndices[];
};

layout (std430, binding = 5) readonly buffer TileInfoBuffer {
    TileInfo tiles[];
};

uniform samplerCube u_shadowCubeMap;
uniform ivec2 u_screenSize;
uniform vec3 u_cameraPos;
uniform float u_farPlane;

const int TILE_SIZE = 16;

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float numinator = a2;
    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = 3.14159265359 * denominator * denominator;

    return numinator / denominator;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

vec3 frenelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float calculateOmniShadow(vec3 lightPos) {
    vec3 fragToLight = v_worldPos - lightPos;
    float closestDepth = texture(u_shadowCubeMap, fragToLight).r * u_farPlane;
    float currentDepth = length(fragToLight);
    float bias = 0.05;

    float shadow = 0.0;
    float samples = 4.0;
    float offset = 0.1;
    for (float x = -offset; x < offset; x += offset / (samples * 0.5)) {
        for (float y = -offset; y < offset; y += offset / (samples * 0.5)) {
            for (float z = -offset; z < offset; z += offset / (samples * 0.5)) {
                float closestDepth = texture(u_shadowCubeMap, fragToLight + vec3(x, y, z)).r;
                closestDepth *= u_farPlane;   // undo mapping [0;1]
                if (currentDepth - bias > closestDepth)
                shadow += 1.0;
            }
        }
    }
    shadow /= (samples * samples * samples);
    return 1.0f - shadow;
}

void main() {
    Material material = materialBuffer.materials[f_materialID];

    vec3 normal = normalize(v_normal);
    vec3 view = normalize(u_cameraPos - v_worldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.albedo.rgb, material.metallic);

    ivec2 pixelCoord = ivec2(floor(gl_FragCoord.xy));
    pixelCoord = clamp(pixelCoord, ivec2(0), u_screenSize - ivec2(1));

    ivec2 tileCoords = ivec2(pixelCoord) / TILE_SIZE;

    int tilesX = (u_screenSize.x + TILE_SIZE - 1) / TILE_SIZE;
    int tilesY = (u_screenSize.y + TILE_SIZE - 1) / TILE_SIZE;

    tileCoords = clamp(tileCoords, ivec2(0), ivec2(tilesX - 1, tilesY - 1));

    int tileIndex = tileCoords.y * tilesX + tileCoords.x;
    TileInfo tile = tiles[tileIndex];

    vec3 totalLigth = vec3(0.0);
    uint offset = tile.offset;

    for (uint i = 0; i < tile.lightsCount; i++) {
        // Light calculations
        uint lightIndex = lightIndices[offset + i];
        vec3 lightPos = lights[lightIndex].position;
        vec3 lightColor = lights[lightIndex].color;
        float lightIntensity = lights[lightIndex].intensity;
        float lightRadius = lights[lightIndex].radius;

        // Shadow
        float shadow = calculateOmniShadow(lightPos);

        if (shadow > 0.0f) {
            vec3 lightView = lightPos - v_worldPos;

            float distance = length(lightView);
            lightView = normalize(lightView);
            vec3 halfDir = normalize(view + lightView);

            // Attenuation
            float attenuation = 1.0 / (distance * distance + 1.0);
            float falloff = clamp(1.0 - pow(distance / lightRadius, 4.0), 0.0, 1.0);
            falloff *= falloff;
            attenuation *= falloff;

            vec3 radiance = lightColor * lightIntensity * attenuation;

            // Cook-Torrance BRDF
            float NDF = distributionGGX(normal, halfDir, material.roughness); // Normal Distribution Function
            float NdotV = max(dot(normal, view), 0.0);
            float NdotL = max(dot(normal, lightView), 0.0);
            float G = geometrySchlickGGX(NdotV, material.roughness) * geometrySchlickGGX(NdotL, material.roughness); // Geometry Function
            vec3 fresnel = frenelSchlick(max(dot(halfDir, lightView), 0.0), F0); // Fresnel

            vec3 kD = vec3(1.0) - fresnel; // Diffuse component
            kD *= 1.0 - material.metallic; // Metals have no diffuse component

            vec3 numerator = NDF * G * fresnel;
            float denominator = 4.0 * max(dot(normal, lightView), 0.0) * max(dot(normal, view), 0.0) + 0.001; // Prevent divide by zero
            vec3 specular = numerator / denominator;

            // Final Color
            vec3 Lo = (kD * material.albedo.rgb / 3.141592653 + specular) * radiance * NdotL; // Outgoing Radiance

            totalLigth += Lo * shadow;
        }
    }

    vec3 ambient = vec3(0.03) * material.albedo.rgb * material.ao;
    vec3 color = ambient + totalLigth;

    // HDR tonemapping and gamma correction
    float gamma = 2.2;
    color = color / (color + vec3(1.0)); // HDR tonemapping
    color = pow(color, vec3(1.0 / gamma)); // Gamma correction

    fragColor = vec4(color, 1.0);
}