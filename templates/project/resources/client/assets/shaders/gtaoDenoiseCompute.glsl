#version 460

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform sampler2D u_sourceTexture;
uniform sampler2D u_depthTexture;

layout (r8, binding = 0) uniform writeonly image2D u_outputImage;

uniform mat4 u_inverseProjection;
uniform ivec2 u_axis; // (1,0) for horizontal, (0,1) for vertical
uniform float u_blurSharpness;

const int KERNEL_RADIUS = 2;

float reconstructViewZ(float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(0.0, 0.0, z, 1.0);
    vec4 viewSpacePosition = u_inverseProjection * clipSpacePosition;
    return viewSpacePosition.z / viewSpacePosition.w;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 screenSize = imageSize(u_outputImage);

    // Bounds check
    if (pixelCoord.x >= screenSize.x || pixelCoord.y >= screenSize.y) {
        return;
    }

    float centerDepth = texelFetch(u_depthTexture, pixelCoord, 0).r;

    // Skip far plane pixels
    if (centerDepth >= 0.99999) {
        imageStore(u_outputImage, pixelCoord, vec4(1.0));
        return;
    }

    float centerViewZ = reconstructViewZ(centerDepth);
    float centerAO = texelFetch(u_sourceTexture, pixelCoord, 0).r;

    float totalWeight = 1.0;
    float accumulatedAO = centerAO;

    for (int i = -KERNEL_RADIUS; i <= KERNEL_RADIUS; i++) {
        if (i == 0) continue;

        ivec2 sampleCoord = pixelCoord + u_axis * i;

        // Bounds check for sample
        if (sampleCoord.x < 0 || sampleCoord.x >= screenSize.x ||
        sampleCoord.y < 0 || sampleCoord.y >= screenSize.y) {
            continue;
        }

        float sampleDepth = texelFetch(u_depthTexture, sampleCoord, 0).r;

        // Skip skybox samples
        if (sampleDepth >= 0.99999) {
            continue;
        }

        float sampleViewZ = reconstructViewZ(sampleDepth);
        float sampleAO = texelFetch(u_sourceTexture, sampleCoord, 0).r;

        // Depth-aware bilateral weight
        float depthDiff = abs(centerViewZ - sampleViewZ);

        // Make the depth sensitivity relative to distance
        float depthSensitivity = u_blurSharpness / max(abs(centerViewZ), 1.0);
        float weight = exp(-depthSensitivity * depthDiff);

        // Spatial weight (Gaussian-like falloff)
        float spatialWeight = exp(- float(i * i) / (2.0 * float(KERNEL_RADIUS * KERNEL_RADIUS)));
        weight *= spatialWeight;

        accumulatedAO += sampleAO * weight;
        totalWeight += weight;
    }

    float finalAO = accumulatedAO / totalWeight;

    imageStore(u_outputImage, pixelCoord, vec4(finalAO));
}