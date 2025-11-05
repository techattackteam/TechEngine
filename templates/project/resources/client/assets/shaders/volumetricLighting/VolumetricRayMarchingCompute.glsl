#version 460 core
#extension GL_NV_uniform_buffer_std430_layout: enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0, rgba16f) uniform image2D u_hdrColor;

layout (binding = 0) uniform sampler3D u_froxelScattering;
layout (binding = 1) uniform sampler2D u_depthBuffer;

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

uniform float u_blendingFactor;

float sliceBoundary(int z) {
    return froxelNearPlane * pow(froxelFarPlane / froxelNearPlane, float(z) / float(froxelDimensions.z));
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);

    vec2 uv = (vec2(pixelCoords) + vec2(0.5)) / vec2(textureSize(u_depthBuffer, 0));
    float raw_depth = texture(u_depthBuffer, uv).r;

    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, raw_depth * 2.0 - 1.0, 1.0);
    vec4 hWorldPos = viewProjectionInverse * clipSpacePosition;
    vec3 worldPos = hWorldPos.xyz / hWorldPos.w;
    float surfaceViewZ = abs((viewMatrix * vec4(worldPos, 1.0)).z);

    vec3 accumulatedScattering = vec3(0.0);
    float accumulatedTransmittance = 1.0;

    for (int step = 0; step < froxelDimensions.z; step++) {
        float sliceStart = sliceBoundary(step);
        float sliceEnd = sliceBoundary(step + 1);

        if (surfaceViewZ <= sliceStart) {
            break;
        }

        // Calculate depth through this froxel slice
        float D = min(sliceEnd, surfaceViewZ) - sliceStart;

        vec3 froxelUVW = vec3(uv, (float(step) + 0.5) / float(froxelDimensions.z));
        vec4 froxelData = texture(u_froxelScattering, froxelUVW);

        vec3 S = froxelData.rgb;  // sigma_s * L_i (scattered radiance)
        float sigma_t = max(froxelData.a, 0.00001);

        // Energy-conserving integration (from EA slides)
        // Integral: ∫₀^D e^(-σₜ·x) × S dx = (S/σₜ) × (1 - e^(-σₜ·D))
        float tau = sigma_t * D;  // Optical depth

        // Clamp optical depth to prevent numerical issues
        tau = min(tau, 20.0);

        float sliceTransmittance = exp(-tau);

        // The energy-conserving integration formula
        vec3 integratedScattering = (S / sigma_t) * (1.0 - sliceTransmittance);

        // Accumulate with transmittance from previous steps
        accumulatedScattering += accumulatedTransmittance * integratedScattering;
        accumulatedTransmittance *= sliceTransmittance;

        // Early exit when transmittance is negligible
        if (accumulatedTransmittance < 0.001) {
            accumulatedTransmittance = 0.0;
            break;
        }

        if (surfaceViewZ <= sliceEnd) {
            break;
        }
    }

    vec4 sceneColor = imageLoad(u_hdrColor, pixelCoords);

    // Composite: background * transmittance + in-scattered light
    vec3 color = sceneColor.rgb * accumulatedTransmittance + accumulatedScattering;

    sceneColor = mix(sceneColor, vec4(color, 1.0), u_blendingFactor);
    imageStore(u_hdrColor, pixelCoords, sceneColor);
}