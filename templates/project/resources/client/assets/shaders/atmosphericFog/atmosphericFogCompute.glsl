#version 460 core

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0, rgba16f) uniform readonly image2D u_hdrBuffer;
layout (binding = 1, rgba16f) uniform writeonly image2D u_outputBuffer;
layout (binding = 2) uniform sampler2D u_depthBuffer;

uniform float u_fogDensity;
uniform float u_fogHeightFalloff;
uniform float u_fogHeight;
uniform float u_fogStart;
uniform float u_fogEnd;
uniform float u_skyboxFogAmount;
uniform int u_fogBlendMode; // 0 = height only, 1 = distance only, 2 = max, 3 = additive

uniform vec3 u_fogColorBase;
uniform vec3 u_fogColorSky;
uniform vec3 u_fogColorSun;
uniform bool u_useDirectionalColor;
uniform float u_sunScatteringIntensity;

uniform float u_mieScattering;
uniform float u_rayleighScattering;

uniform vec3 u_cameraPosition;
uniform mat4 u_inverseViewProjection;
uniform vec3 u_sunDirection;


vec3 reconstructWorldPosition(vec2 uv, float depth) {
    // NDC coordinates
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    // World space
    vec4 worldPos = u_inverseViewProjection * ndc;
    worldPos /= worldPos.w;

    return worldPos.xyz;
}

// Based on: https://iquilezles.org/articles/fog/
float getHeightFogAmount(vec3 rayOrigin, vec3 rayDirection, float distance) {
    float fogAmount = 0.0;

    if (u_fogHeightFalloff > 0.001f) {
        float startHeight = rayOrigin.y - u_fogHeight;
        float endHeight = (rayOrigin.y + rayDirection.y * distance) - u_fogHeight;


        if (abs(rayDirection.y) > 0.001f) {
            fogAmount = (exp(-startHeight * u_fogHeightFalloff) - exp(-endHeight * u_fogHeightFalloff)) / (rayDirection.y * u_fogHeightFalloff);
        } else {
            fogAmount = distance * exp(-startHeight * u_fogHeightFalloff);
        }

        fogAmount *= u_fogDensity;
    } else {
        fogAmount = distance * u_fogDensity;
    }

    return fogAmount;
}

float getDistanceFog(float distance) {
    if (distance < u_fogStart) {
        return 0.0;
    }

    if (distance >= u_fogEnd) {
        return 1.0;
    }

    float fogRange = u_fogEnd - u_fogStart;
    float distanceIntoFog = distance - u_fogStart;
    float normalizedDistance = distanceIntoFog / fogRange;

    return clamp(normalizedDistance, 0.0, 1.0);
}

float getMiePhase(float cosTheta, float g) {
    float g2 = g * g;
    float nom = (1.0 - g2);
    float denom = 4.0 * 3.14159265359 * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
    return nom / denom;
}

float getRayleighPhase(float cosTheta) {
    return (3.0 / (16.0 * 3.14159265359)) * (1.0 + cosTheta * cosTheta);
}

vec3 getFogColor(vec3 viewDir, float heighFactor) {
    vec3 fogColor = mix(u_fogColorBase, u_fogColorSky, heighFactor);

    if (u_useDirectionalColor) {
        float cosTheta = dot(viewDir, u_sunDirection);

        float mie = getMiePhase(cosTheta, 0.76) * u_mieScattering;

        float rayleigh = getRayleighPhase(cosTheta) * u_rayleighScattering;

        float sunInfluence = pow(max(cosTheta, 0.0), 8.0) * u_sunScatteringIntensity;
        fogColor = mix(fogColor, u_fogColorSun, sunInfluence);
        fogColor += u_fogColorSun * (mie + rayleigh);
    }
    return fogColor;
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imageSize = imageSize(u_hdrBuffer);

    if (pixelCoords.x >= imageSize.x || pixelCoords.y >= imageSize.y) {
        return;
    }

    vec2 uv = (vec2(pixelCoords) + vec2(0.5)) / vec2(imageSize);

    vec3 albedo = imageLoad(u_hdrBuffer, pixelCoords).rgb;
    float depth = texture(u_depthBuffer, uv).r;

    vec3 worldPosition = reconstructWorldPosition(uv, depth);
    if (depth >= 0.9999f) {
        vec3 viewDirection = normalize(worldPosition - u_cameraPosition);

        float horizonFactor = 1.0 - abs(viewDirection.y);
        horizonFactor = pow(horizonFactor, 2.0);

        float skyFogAmount = horizonFactor * u_skyboxFogAmount * 5.0;

        vec3 fogColor = getFogColor(viewDirection, 0.0f);
        vec3 finalColor = mix(albedo, fogColor, clamp(skyFogAmount, 0.0, 0.8));

        imageStore(u_outputBuffer, pixelCoords, vec4(finalColor, 1.0));
        return;
    }

    vec3 rayDirection = normalize(worldPosition - u_cameraPosition);
    float distance = length(worldPosition - u_cameraPosition);

    float heightFogAmount = getHeightFogAmount(u_cameraPosition, rayDirection, distance);
    float distanceFogAmount = getDistanceFog(distance);

    float fogAmount = 0.0;
    if (u_fogBlendMode == 0) {
        fogAmount = heightFogAmount;
    } else if (u_fogBlendMode == 1) {
        fogAmount = distanceFogAmount;
    } else if (u_fogBlendMode == 2) {
        fogAmount = max(heightFogAmount, distanceFogAmount);
    } else if (u_fogBlendMode == 3) {
        fogAmount = clamp(heightFogAmount + distanceFogAmount, 0.0, 1.0);
    } else if (u_fogBlendMode == 4) {
        fogAmount = heightFogAmount * distanceFogAmount;
    } else {
        fogAmount = max(heightFogAmount, distanceFogAmount);
    }

    fogAmount = clamp(fogAmount, 0.0, 1.0);

    float heightFactor = clamp((worldPosition.y - u_fogHeight) / 100.0f, 0.0, 1.0);

    vec3 fogColor = getFogColor(rayDirection, heightFactor);

    vec3 finalColor = mix(albedo, fogColor, fogAmount);

    imageStore(u_outputBuffer, pixelCoords, vec4(finalColor, 1.0));
}