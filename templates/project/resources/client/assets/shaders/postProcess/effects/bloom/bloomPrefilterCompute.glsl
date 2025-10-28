#version 460 core

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (binding = 0) uniform sampler2D u_hdrBuffer;

layout (binding = 0, rgba16f) uniform writeonly image2D u_bloomBuffer;

uniform float u_threshold = 1.5;
uniform float u_knee = 0.5;

vec3 softThreshold(vec3 color, float threshold, float knee) {
    float brightness = max(color.r, max(color.g, color.b)); // or use luminance

    // Calculate soft curve
    float softness = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    softness = (softness * softness) / (4.0 * knee + 0.00001);

    // Calculate contribution
    float contribution = max(softness, brightness - threshold);
    contribution /= max(brightness, 0.00001);

    return color * contribution;
}

void main() {

    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dimensions = imageSize(u_bloomBuffer);

    if (pixelCoords.x >= dimensions.x || pixelCoords.y >= dimensions.y) {
        return;
    }

    ivec2 srcPixelCoords = pixelCoords * 2;
    vec3 color = texelFetch(u_hdrBuffer, pixelCoords, 0).rgb;

    // Apply the brightness threshold
    vec3 bloom = softThreshold(color, u_threshold, u_knee);

    // Store the result in the same pixel coordinate of the destination
    imageStore(u_bloomBuffer, pixelCoords, vec4(bloom, 1.0));
}