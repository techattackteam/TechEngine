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

layout (std430, binding = 2) buffer MaterialBuffer {
    Material materials[];
} materialBuffer;

// Hardcoded camera and light positions for simplicity
uniform vec3 u_cameraPos;
uniform vec3 u_lightPos = vec3(5.0, 5.0, 5.0);
uniform vec3 u_lightColor = vec3(150.0, 150.0, 150.0); // A very bright light!

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265359 * denom * denom;

    return num / denom;
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

void main() {
    Material material = materialBuffer.materials[f_materialID];

    vec3 normal = normalize(v_normal);
    vec3 view = normalize(u_cameraPos - v_worldPos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.albedo.rgb, material.albedo.a);

    // Light calculations

    vec3 lightView = normalize(u_lightPos - v_worldPos);
    vec3 halfDir = normalize(view + lightView);

    // Attenuation

    float distance = length(u_lightPos - v_worldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = u_lightColor * attenuation;

    // Cook-Torrance BRDF

    float NDF = distributionGGX(normal, halfDir, material.roughness); // Normal Distribution Function
    float NdotV = max(dot(normal, lightView), 0.0);
    float geometryFunction = geometrySchlickGGX(NdotV, material.roughness); // Geometry Function
    vec3 fresnel = frenelSchlick(max(dot(halfDir, lightView), 0.0), F0); // Fresnel

    vec3 kD = vec3(1.0) - fresnel; // Diffuse component
    kD *= 1.0 - material.metallic; // Metals have no diffuse component

    vec3 numerator = NDF * geometryFunction * fresnel;
    float denominator = 4.0 * max(dot(normal, lightView), 0.0) * max(dot(normal, view), 0.0) + 0.001; // Prevent divide by zero
    vec3 specular = numerator / denominator;

    // Final Color
    float NdotL = max(dot(normal, view), 0.0);
    vec3 Lo = (kD * material.albedo.rgb / 3.141592653 + specular) * radiance * NdotL; // Outgoing Radiance

    vec3 ambient = vec3(0.03) * material.albedo.rgb * material.ao;
    vec3 color = ambient + Lo;

    // HDR tonemapping and gamma correction
    float gamma = 2.2;
    color = color / (color + vec3(1.0)); // HDR tonemapping
    color = pow(color, vec3(1.0 / gamma)); // Gamma correction

    fragColor = vec4(color, 1.0);
}