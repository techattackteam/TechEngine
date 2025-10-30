#version 460 core
#extension GL_NV_uniform_buffer_std430_layout: enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0, rgba16f) uniform image3D u_volumetricLightingVolume;

//layout (binding = 0) uniform sampler2D u_hdrBuffer;
layout (binding = 0) uniform sampler2D u_depthBuffer;
layout (binding = 1) uniform sampler3D u_froxelScattering;

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

uniform float u_nearPlane;
uniform float u_farPlane;

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
    float depthRange = froxelFarPlane - froxelNearPlane;

    if (useExponentialDepth != 0u) {
        float scale = depthDistributionScale;
        float linearZ = log(normalizedZ * (exp(scale) - 1.0) + 1.0) / scale;
        return froxelNearPlane + linearZ * depthRange;
    } else {
        return froxelNearPlane + normalizedZ * depthRange;
    }
}

uvec3 screenToFroxel(vec2 screenUV, float linearDepth) {
    uvec3 froxel;
    froxel.x = uint(screenUV.x * float(froxelDimensions.x));
    froxel.y = uint(screenUV.y * float(froxelDimensions.y));
    froxel.z = uint(depthToFroxelZ(linearDepth));

    froxel = clamp(froxel, uvec3(0), froxelDimensions - 1u);

    return froxel;
}

float interleavedGradientNoise(vec2 screenPos, float time) {
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    float t = fract(time);
    return fract(magic.z * fract(dot(screenPos + vec2(t * 13.7, t * 7.3), magic.xy)));
}

float slice_distance(int z)
{
    return froxelNearPlane * pow(froxelFarPlane / froxelNearPlane, (float(z) + 0.5f) / float(128));
}

float slice_thickness(int z)
{
    return abs(slice_distance(z + 1) - slice_distance(z));
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 screenSize = imageSize(u_volumetricLightingVolume).xy;

    if (pixelCoords.x >= screenSize.x || pixelCoords.y >= screenSize.y) {
        return;
    }

    vec2 uv = (vec2(pixelCoords) + vec2(0.5)) / vec2(screenSize);

    //vec3 sceneColor = imageLoad(u_volumetricLightingVolume, pixelCoords).rgb;
    float depthNonLinear = texture(u_depthBuffer, uv).r;
    depthNonLinear = depthNonLinear * 2.0 - 1.0; // Convert from [0,1] to NDC [-1,1]
    float depthLinear = 0.0f;
    if (depthNonLinear >= 0.9999) {
        depthLinear = froxelFarPlane;
    } else {
        depthLinear = (2.0 * u_farPlane * u_nearPlane) / (u_farPlane + u_nearPlane - depthNonLinear * (u_farPlane - u_nearPlane));
    }
    float startDepth = froxelNearPlane;
    float endDepth = froxelFarPlane;
    const int numSteps = 128;
    float stepSize = (endDepth - startDepth) / float(numSteps);


/**    if (stepSize <= 0.0 || endDepth <= startDepth) {
        imageStore(u_volumetricLightingVolume, pixelCoords, vec4(sceneColor, 1.0));
        return;
    }*/
    vec3 accumulatedScattering = vec3(0.0);
    float accumulatedTransmittance = 1.0;

    for (int step = 0; step < 128; step++) {
        ivec3 coord = ivec3(gl_GlobalInvocationID.xy, step);
        vec4 froxelData = texelFetch(u_froxelScattering, coord, 0);

        vec3 scattering = froxelData.rgb;
        float extinction = froxelData.a;

        const float thickness = slice_thickness(step);
        float transmittance = exp(-extinction * thickness);

        // Accumulate in-scattering weighted by transmittance
        // This is the analytical solution to the integral
        float weight = (1.0 - transmittance) / extinction;
        accumulatedScattering += scattering * accumulatedTransmittance * weight;

        // Accumulate transmittance (multiply along the ray)
        accumulatedTransmittance *= transmittance;
    /**

        const float slice_transmittance = exp(-extinction * thickness);

        vec3 slice_scattering_integral = scattering * (1.0 - slice_transmittance) / extinction;

        accumulatedScattering += slice_scattering_integral * accumulatedTransmittance;
        accumulatedTransmittance *= slice_transmittance;*/

        imageStore(u_volumetricLightingVolume, coord, vec4(accumulatedScattering, accumulatedTransmittance));
    }
}