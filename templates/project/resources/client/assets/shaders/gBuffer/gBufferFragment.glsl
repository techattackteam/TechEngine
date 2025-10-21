#version 460 core

#extension GL_ARB_bindless_texture: enable

layout (location = 0) out vec4 out_albedo;
layout (location = 1) out vec4 out_normal;

in vec3 v_worldPos;
in vec3 v_viewPos;
in vec3 v_normal;
in vec2 v_textCoord;
in flat uint v_materialIndex;

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

layout (std430, binding = 2) buffer MaterialBuffer {
    Material materials[];
};


void main() {
    Material material = materials[v_materialIndex];
    vec3 albedo;
    if (material.albedoHandle != uvec2(0)) {
        sampler2D albedoTexture = sampler2D(material.albedoHandle);
        albedo = texture(albedoTexture, v_textCoord).rgb;
    } else {
        albedo = material.albedo.rgb;
    }

    float encodedMaterialID = float(v_materialIndex) / 255.0;
    out_albedo = vec4(albedo, encodedMaterialID);


    vec3 worldNormal = normalize(v_normal);

    out_normal = vec4(worldNormal, 1.0);
}
