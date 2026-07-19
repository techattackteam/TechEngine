#pragma once

#include <glm/glm.hpp>

namespace TechEngine {
    struct AOProperties {
        bool enabled = false;
        int directionCount = 12;
        int stepsPerDirection = 16;
        float radius = 1.0f;
        float thickness = 0.5f;
    };

    struct BloomProperties {
        bool enabled = false;
        float threshold = 1.5f;
        float knee = 0.5f;
        float intensity = 1.0f;
    };

    struct ChromaticAberrationProperties {
        bool enabled = false;
        float strength = 0.005f;
        float offset = 1.0f;
    };

    struct VignetteProperties {
        bool enabled = false;
        float strength = 1.0f;
        float power = 1.5f;
    };

    struct GammaProperties {
        bool enabled = false;
        glm::vec3 lift = glm::vec3(0.0f);
        float liftIntensity = 1.0f;

        float gamma = 2.2f;
        glm::vec3 gammaRGB = glm::vec3(1.0f);
        float gammaIntensity = 1.0f;

        glm::vec3 gain = glm::vec3(1.0f);
        float gainIntensity = 1.0f;
    };

    struct ColorGradingProperties {
        float exposure = 1.0f;
        float saturation = 1.0f;
        float contrast = 1.0f;
        float brightness = 1.0f;

        bool useLUT = true;
        float lutStrength = 1.0f;
    };

    struct FilmGrainProperties {
        bool filmGrainEnabled = false;
        float filmGrainIntensity = 0.05f;
        float filmGrainSize = 1.0f;
    };

    struct FogProperties {
        bool enabled = false;

        float fogDensity = 0.01f;
        float fogHeightFalloff = 0.1f;
        float fogHeight = 0.0f;

        float fogStart = 0.0f;
        float fogEnd = 1.0f;
        float skyboxFogAmount = 0.5f;
        int fogBlendMode = 0;
        glm::vec3 fogColorBase = glm::vec3(0.5f, 0.5f, 0.5f);
        glm::vec3 fogColorSky = glm::vec3(0.7f, 0.7f, 0.8f);

        bool useDirectionalColor = false;
        glm::vec3 fogColorSun = glm::vec3(1.0f, 1.0f, 0.9f);
        float sunScatteringIntensity = 1.0f;

        float mieScattering = 1.0f;
        float rayleighScattering = 1.0f;
    };

    struct FroxelGridProperties {
        // Grid dimensions
        uint32_t width = 160;
        uint32_t height = 90;
        uint32_t depth = 128;

        // Depth distribution
        float nearPlane = 0.1f; // Camera near plane
        float farPlane = 100.0f; // Camera far plane
        bool useExponentialDepth = true;

        float depthDistributionScale = 1.0f;
    };

    struct FroxelParams {
        glm::mat4 viewProjectionInverse;
        glm::mat4 viewMatrix;

        glm::uvec3 froxelDimensions;
        float froxelNearPlane;

        float froxelFarPlane;
        float depthDistributionScale;
        uint32_t useExponentialDepth;
        float rcpFroxelDimX;

        float rcpFroxelDimY;
        float rcpFroxelDimZ;
        glm::vec2 padding;

        glm::vec3 cameraPosition;
        bool myImplementation = true;
    };

    struct VolumetricSettings {
        glm::vec3 scatteringCoefficient = glm::vec3(0.1f, 0.1f, 0.1f);
        float density = 5.0f;

        glm::vec3 absorptionCoefficient = glm::vec3(0.0f);
        float anisotropy = 0.0f;

        glm::vec3 emissiveCoefficient = glm::vec3(0.0f);
        bool enabled = false;

        float blendingFactor = 1.0f;
        glm::vec3 padding;
    };

    // Aggregate of all renderer tunables. Renderer owns one instance and hands
    // out references to individual members via its getXProperties() accessors.
    struct RenderSettings {
        AOProperties ao;
        BloomProperties bloom;
        ChromaticAberrationProperties chromaticAberration;
        VignetteProperties vignette;
        GammaProperties gamma;
        ColorGradingProperties colorGrading;
        FilmGrainProperties filmGrain;
        FogProperties fog;
        FroxelGridProperties froxelGrid;
        FroxelParams froxelParams;
        VolumetricSettings volumetric;
    };
}
