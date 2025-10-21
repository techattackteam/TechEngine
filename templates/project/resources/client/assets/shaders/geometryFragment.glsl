#version 460 core
#extension GL_ARB_bindless_texture: enable

#define SHADOW_CASCADE_COUNT 4

layout (location = 0) out vec4 out_fragColor;
layout (location = 1) out vec4 out_screenLight;

in vec3 v_worldPos;
in vec3 v_normal;
in vec2 v_textCoord;
in flat uint f_materialID;
in vec4 v_viewPos;

struct Material {
    vec4 albedo;

    float metallic;
    float roughness;
    float ao;
    float pad_0;

    vec4 emission;

    uvec2 albedoHandle;
    uvec2 normalHandle;

    uvec2 metallicHandle;
    uvec2 roughnessHandle;

    uvec2 ambientOcclusionHandle;
    uvec2 emissionHandle;
};

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

uniform ivec2 u_screenSize;
uniform vec3 u_cameraPos;
uniform float u_farPlane;
uniform samplerCube u_irradianceMap;
uniform samplerCube u_prefilterMap;
uniform sampler2D u_brdfLUT;
uniform sampler2D u_aoMap;

const int TILE_SIZE = 16;

void sampleMaterialTextures(inout Material material, vec2 uv, inout vec3 normal) {
    if (material.albedoHandle != uvec2(0)) {
        sampler2D albedoSampler = sampler2D(material.albedoHandle);
        vec4 albedoTex = texture(albedoSampler, v_textCoord);
        material.albedo = albedoTex;
    }
    if (material.metallicHandle != uvec2(0)) {
        sampler2D metallicSampler = sampler2D(material.metallicHandle);
        float metallicTex = texture(metallicSampler, v_textCoord).r;
        material.metallic = metallicTex;
    }
    if (material.roughnessHandle != uvec2(0)) {
        sampler2D roughnessSampler = sampler2D(material.roughnessHandle);
        float roughnessTex = texture(roughnessSampler, v_textCoord).r;
        material.roughness = roughnessTex;
    }
    if (material.ambientOcclusionHandle != uvec2(0)) {
        sampler2D aoSampler = sampler2D(material.ambientOcclusionHandle);
        float aoTex = texture(aoSampler, v_textCoord).r;
        material.ao = aoTex;
    }
    if (material.emissionHandle != uvec2(0)) {
        sampler2D emissionSampler = sampler2D(material.emissionHandle);
        vec3 emissionTex = texture(emissionSampler, v_textCoord).rgb;
        material.emission = vec4(emissionTex, 1.0);
    }

    if (material.normalHandle != uvec2(0)) {
        sampler2D normalSampler = sampler2D(material.normalHandle);
        vec3 normalTex = texture(normalSampler, v_textCoord).rgb;
        normalTex = normalTex * 2.0 - 1.0; // Transform from [0,1] to [-1,1]
        normalTex = normalize(normalTex);

        // Tangent space to world space
        vec3 tangent;
        vec3 bitangent;
        vec3 N = normalize(v_normal);
        if (abs(N.z) < 0.999) {
            tangent = normalize(cross(N, vec3(0.0, 0.0, 1.0)));
        } else {
            tangent = normalize(cross(N, vec3(0.0, 1.0, 0.0)));
        }
        bitangent = cross(N, tangent);

        mat3 TBN = mat3(tangent, bitangent, N);
        normal = normalize(TBN * normalTex);
    }
}

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

vec3 frenelSchlick(float cosTheta, vec3 F0, float roughness) {
    return F0 + max(vec3(1.0 - roughness), 1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float calculateOmniShadow(Light light) {
    vec3 fragToLight = v_worldPos - light.position;
    samplerCube handle = samplerCube(light.shadowHandle[0]);
    float closestDepth = texture(handle, fragToLight).r * u_farPlane;
    float currentDepth = length(fragToLight);
    float bias = max(0.15 * (1.0 - dot(v_normal, light.direction)), 0.005);
    bias = 0.0f;
    float shadow = currentDepth > (closestDepth + bias) ? 1.0 : 0.0;

    return 1.0f - shadow;
}

float calculateDepthShadow(Light light) {
    vec4 fragToLight = light.lightSpaceMatrix[0] * vec4(v_worldPos, 1.0f);
    sampler2D handle = sampler2D(light.shadowHandle[0]);

    vec3 coords = fragToLight.xyz / fragToLight.w;
    coords = coords * vec3(0.5, 0.5, 0.5) + vec3(0.5);

    float closestDepth = texture(handle, coords.xy).r;
    float currentDepth = coords.z;
/**    if (coords.z > 1.0) {
        return 1.0f;
    }*/
    //float bias = max(0.05 * (1.0 - dot(v_normal, light.direction)), 0.0005);
    float shadow = currentDepth > (closestDepth) ? 1.0 : 0.0;
    return 1.0f - shadow;
}

float calculateCascadeDepthShadow(Light light) {
    float fragmentDepth = -v_viewPos.z; // In view space, Z is negative going into the screen

    int cascadeIndex = 0;
    for (int i = 0; i < SHADOW_CASCADE_COUNT; ++i) {
        if (fragmentDepth > light.cascadeSplits[i]) {
            cascadeIndex = i + 1;
        }
    }
    mat4 lightMatrix = light.lightSpaceMatrix[cascadeIndex];
    vec4 fragToLight = lightMatrix * vec4(v_worldPos, 1.0);
    sampler2D handle = sampler2D(light.shadowHandle[cascadeIndex]);

    vec3 coords = fragToLight.xyz / fragToLight.w;
    coords = coords * vec3(0.5, 0.5, 0.5) + vec3(0.5);

    float closestDepth = texture(handle, coords.xy).r;
    float currentDepth = coords.z;
/**    if (coords.z > 1.0 || coords.z < -1.0) {
        return 1.0f;
    }*/
    //float bias = max(0.05 * (1.0 - dot(v_normal, light.direction)), 0.0005);
    float shadow = currentDepth > (closestDepth) ? 1.0 : 0.0;
    return 1.0f - shadow;
}

vec3 PBRCalculate(Light light, Material material, vec3 radiance, vec3 normal, vec3 viewDirection, vec3 lightDirection) {
    // Base reflectivity at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.albedo.rgb, material.metallic);

    lightDirection = normalize(lightDirection);
    vec3 halfDir = normalize(viewDirection + lightDirection);

    // Cook-Torrance BRDF
    float NDF = distributionGGX(normal, halfDir, material.roughness); // Normal Distribution Function
    float NdotV = max(dot(normal, viewDirection), 0.0);
    float NdotL = max(dot(normal, lightDirection), 0.0);
    float G = geometrySchlickGGX(NdotV, material.roughness) * geometrySchlickGGX(NdotL, material.roughness); // Geometry Function
    vec3 fresnel = frenelSchlick(max(dot(halfDir, lightDirection), 0.0), F0, 0.0f); // Fresnel

    vec3 kD = vec3(1.0) - fresnel; // Diffuse component
    kD *= 1.0 - material.metallic; // Metals have no diffuse component

    vec3 numerator = NDF * G * fresnel;
    float denominator = 4.0 * max(dot(normal, lightDirection), 0.0) * max(dot(normal, viewDirection), 0.0) + 0.001; // Prevent divide by zero
    vec3 specular = numerator / denominator;

    // Final Color
    vec3 Lo = (kD * material.albedo.rgb / 3.141592653 + specular) * radiance * NdotL; // Outgoing Radiance

    return Lo /*** shadow*/;
}


vec3 calculateSpotLight(Light light, Material material, vec3 normal, vec3 view) {
    vec3 lightDir = light.position - v_worldPos;
    float dist = length(lightDir);
    lightDir = normalize(lightDir);

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.innerCutoff - light.outerCutoff;
    float attenuation = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

    vec3 radiance = light.color * light.intensity * attenuation;

    return PBRCalculate(light, material, radiance, normal, view, lightDir);
}

vec3 calculatePointLight(Light light, Material material, vec3 normal, vec3 view) {
    vec3 lightDir = light.position - v_worldPos;
    float dist = length(lightDir);
    lightDir = normalize(lightDir);

    float attenuation = 1.0 / (dist * dist + 1.0);
    float falloff = clamp(1.0 - pow(dist / light.radius, 4.0), 0.0, 1.0);
    falloff *= falloff;
    attenuation *= falloff;

    vec3 radiance = light.color * light.intensity * attenuation;

    return PBRCalculate(light, material, radiance, normal, view, lightDir);
}

vec3 calculateDirectionalLight(Light light, Material Material, vec3 normal, vec3 view) {
    vec3 lightDir = normalize(-light.direction);
    vec3 radiance = light.color * (light.intensity / 100.0);

    return PBRCalculate(light, Material, radiance, normal, view, lightDir);
}

void main() {
    Material material = materialBuffer.materials[f_materialID];

    vec3 normal = normalize(v_normal);
    vec3 view = normalize(u_cameraPos - v_worldPos);


    // 1. Sample Material Textures
    sampleMaterialTextures(material, v_textCoord, normal);

    // 2. Sample Ambient Occlusion Map
    vec2 screenUV = gl_FragCoord.xy / vec2(u_screenSize);

    ivec2 pixelCoord = ivec2(floor(gl_FragCoord.xy));
    pixelCoord = clamp(pixelCoord, ivec2(0), u_screenSize - ivec2(1));

    ivec2 tileCoords = ivec2(pixelCoord) / TILE_SIZE;

    int tilesX = (u_screenSize.x + TILE_SIZE - 1) / TILE_SIZE;
    int tilesY = (u_screenSize.y + TILE_SIZE - 1) / TILE_SIZE;

    tileCoords = clamp(tileCoords, ivec2(0), ivec2(tilesX - 1, tilesY - 1));

    int tileIndex = tileCoords.y * tilesX + tileCoords.x;
    TileInfo tile = tiles[tileIndex];

    vec3 directLight = vec3(0.0);
    uint offset = tile.offset;

    // 3. Calculate Direct Lighting
    for (uint i = 0; i < tile.lightsCount; i++) {
        // Light calculations
        uint lightIndex = lightIndices[offset + i];
        Light light = lights[lightIndex];
        vec3 lightPos = light.position;
        vec3 lightColor = light.color;
        float lightIntensity = light.intensity;
        float lightRadius = light.radius;

        float shadow = 1.0f;
        if (light.type == 0) {
            if (light.shadowHandle[0] != uvec2(0)) {
                shadow = calculateOmniShadow(light);
            }
            directLight = directLight + (calculatePointLight(light, material, normal, view) * shadow);

        } else if (light.type == 1) {
            if (light.shadowHandle[0] != uvec2(0)) {
                shadow = calculateCascadeDepthShadow(light);
            }
            directLight = directLight + (calculateDirectionalLight(light, material, normal, view) * shadow);
        } else if (light.type == 2) {
            if (light.shadowHandle[0] != uvec2(0)) {
                shadow = calculateDepthShadow(light);
            }
            directLight = directLight + (calculateSpotLight(light, material, normal, view) * shadow);
        }
    }


    // 4. Calculate Ambient Lighting (IBL)
    float ao = material.ao * texture(u_aoMap, screenUV).a;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.albedo.rgb, material.metallic);

    vec3 F = frenelSchlick(max(dot(normal, view), 0.0), F0, 0.0f);

    vec3 ks = F;
    vec3 kd = vec3(1.0) - ks;
    kd *= 1.0 - material.metallic;

    // Diffuse lighting
    vec3 irradiance = texture(u_irradianceMap, normal).rgb;
    vec3 diffuseIBL = irradiance * material.albedo.rgb;
    vec3 diffuse = kd * diffuseIBL;

    // Specular lighting
    vec3 R = reflect(-view, normal);
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(u_prefilterMap, R, material.roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(u_brdfLUT, vec2(max(dot(normal, view), 0.0), material.roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    // Ambient
    vec3 ambient = specular + diffuse;

    // Emission
    vec3 finalEmission = material.emission.rgb * material.emission.a;

    vec3 color = (ambient + directLight) * ao + finalEmission;

    out_fragColor = vec4(color, 1.0);
    out_screenLight = vec4(1.0);
}