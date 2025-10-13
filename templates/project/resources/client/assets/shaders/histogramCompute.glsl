#version 460 core

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (binding = 0, rgba32f) uniform readonly image2D u_hdrImage;

layout (std430, binding = 0) buffer HistogramBuffer {
    uint u_histogram[];
};
#define EPSILON 0.0001 // Small value to avoid log(0)

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dimensions = imageSize(u_hdrImage);

    if (pixelCoords.x >= dimensions.x || pixelCoords.y >= dimensions.y) {
        return; // Out of bounds
    }

    vec3 hdrColor = imageLoad(u_hdrImage, pixelCoords).rgb;
    float luminance = max(dot(hdrColor, vec3(0.2126, 0.7152, 0.0722)), EPSILON); // Avoid log(0)
    float logLuminance = log2(luminance);

    float minLogLum = -10.0; // Adjust based on expected luminance range
    float maxLogLum = 10.0;  // Adjust based on expected luminance range

    int binIndex = int(((logLuminance - minLogLum) / (maxLogLum - minLogLum)) * 255.0);

    if (binIndex >= 0 && binIndex < 256) {
        atomicAdd(u_histogram[binIndex], 1);
    }
}