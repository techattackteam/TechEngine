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

float interleavedGradientNoise(vec2 screenPos, float time) {
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    float t = fract(time);
    return fract(magic.z * fract(dot(screenPos + vec2(t * 13.7, t * 7.3), magic.xy)));
}

float sliceDistance(int z) {
    return froxelNearPlane * pow(froxelFarPlane / froxelNearPlane, (float(z) + 0.5f) / float(froxelDimensions.z));
}

float sliceTickness(int z) {
    return abs(sliceDistance(z + 1) - sliceDistance(z));
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
        float currentSliceZ = sliceDistance(step);
        if (surfaceViewZ < currentSliceZ) {
            break;
        }
        vec3 froxelUVW = vec3(uv, (float(step) + 0.5) / float(froxelDimensions.z));

        vec4 froxelData = texture(u_froxelScattering, froxelUVW);

        vec3 scattering = froxelData.rgb;
        float extinction = froxelData.a;

        const float thickness = sliceTickness(step);
        float transmittance = exp(-extinction * thickness);

        float weight = (1.0 - transmittance) / extinction;
        accumulatedScattering += scattering * accumulatedTransmittance * weight;

        accumulatedTransmittance *= transmittance;

    }
    vec4 sceneColor = imageLoad(u_hdrColor, pixelCoords);

    sceneColor.rgb = sceneColor.rgb * accumulatedTransmittance + accumulatedScattering;

    imageStore(u_hdrColor, pixelCoords, sceneColor);
}