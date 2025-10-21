#version 450

layout (local_size_x = 8, local_size_y = 8) in;

layout (binding = 0) uniform sampler2D u_currentAO;
layout (binding = 1) uniform sampler2D u_historyAO;
layout (binding = 2) uniform sampler2D u_motionVectors;
layout (binding = 3) uniform sampler2D u_depthTexture;

uniform ivec2 u_screenSize;
uniform float u_temporalAlpha;  // Blend factor (0.1 for exponential, 1.0 to disable)
uniform mat4 u_inverseProjection;

layout (binding = 4, rgba16f) uniform writeonly image2D u_outAO;

// Reconstruct view-space position
vec3 reconstructViewPos(vec2 uv, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = u_inverseProjection * clipPos;
    return viewPos.xyz / viewPos.w;
}

// RGB to YCoCg color space for better comparison
vec3 rgbToYCoCg(vec3 rgb) {
    float Y = dot(rgb, vec3(0.25, 0.5, 0.25));
    float Co = dot(rgb, vec3(0.5, 0.0, -0.5));
    float Cg = dot(rgb, vec3(-0.25, 0.5, -0.25));
    return vec3(Y, Co, Cg);
}

vec3 yCoCgToRgb(vec3 ycocg) {
    float tmp = ycocg.x - ycocg.z;
    return vec3(tmp + ycocg.y, ycocg.x + ycocg.z, tmp - ycocg.y);
}

// Catmull-Rom filtering for better history sampling
vec4 catmullRomFilter(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec2 position = uv / texelSize;
    vec2 centerPosition = floor(position - 0.5) + 0.5;
    vec2 f = position - centerPosition;
    vec2 f2 = f * f;
    vec2 f3 = f2 * f;

    vec2 w0 = f2 - 0.5 * (f3 + f);
    vec2 w1 = 1.5 * f3 - 2.5 * f2 + 1.0;
    vec2 w3 = 0.5 * (f3 - f2);
    vec2 w2 = 1.0 - w0 - w1 - w3;

    vec2 w12 = w1 + w2;
    vec2 tc0 = (centerPosition - 1.0) * texelSize;
    vec2 tc12 = (centerPosition + w2 / w12) * texelSize;
    vec2 tc3 = (centerPosition + 2.0) * texelSize;

    vec4 result =
    texture(tex, vec2(tc0.x, tc0.y)) * (w0.x * w0.y) +
    texture(tex, vec2(tc0.x, tc12.y)) * (w0.x * w12.y) +
    texture(tex, vec2(tc0.x, tc3.y)) * (w0.x * w3.y) +
    texture(tex, vec2(tc12.x, tc0.y)) * (w12.x * w0.y) +
    texture(tex, vec2(tc12.x, tc12.y)) * (w12.x * w12.y) +
    texture(tex, vec2(tc12.x, tc3.y)) * (w12.x * w3.y) +
    texture(tex, vec2(tc3.x, tc0.y)) * (w3.x * w0.y) +
    texture(tex, vec2(tc3.x, tc12.y)) * (w3.x * w12.y) +
    texture(tex, vec2(tc3.x, tc3.y)) * (w3.x * w3.y);

    return result;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoord.x >= u_screenSize.x || pixelCoord.y >= u_screenSize.y) {
        return;
    }

    vec2 uv = (vec2(pixelCoord) + 0.5) / vec2(u_screenSize);
    vec2 texelSize = 1.0 / vec2(u_screenSize);

    float currentDepth = texture(u_depthTexture, uv).r;
    if (currentDepth >= 0.9999) {
        // This is a skybox pixel, no AO needed.
        imageStore(u_outAO, pixelCoord, vec4(1.0));
        return;
    }

    // Get current frame AO
    vec4 currentAO = texture(u_currentAO, uv);

    // Get motion vector and compute history UV
    vec2 motion = texture(u_motionVectors, uv).xy;
    vec2 historyUV = uv - motion;
    vec4 debugColor = vec4(0.0, 1.0, 0.0, 1.0);
    // Check if history is valid (within screen bounds)
    bool validHistory = historyUV.x >= 0.0 && historyUV.x <= 1.0 && historyUV.y >= 0.0 && historyUV.y <= 1.0;

    vec4 finalAO = currentAO;

    vec4 historyAO = catmullRomFilter(u_historyAO, historyUV, texelSize);

    if (validHistory) {
        // Sample history with Catmull-Rom for better quality

        // ============================================================
        // Neighborhood clamping to reject invalid history
        // Sample 3x3 neighborhood of current frame
        // ============================================================
        vec4 neighborMin = vec4(1.0);
        vec4 neighborMax = vec4(0.0);
        vec4 neighborMean = vec4(0.0);

        // Sample 3x3 neighborhood
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                vec2 neighborUV = uv + vec2(x, y) * texelSize;
                vec4 neighborAO = texture(u_currentAO, neighborUV);
                neighborMin = min(neighborMin, neighborAO);
                neighborMax = max(neighborMax, neighborAO);
            }
        }


        neighborMean /= 9.0;

        // Variance clipping (better than min/max clamping)
        vec4 neighborStdDev = vec4(0.0);
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                vec2 neighborUV = uv + vec2(x, y) * texelSize;
                vec4 neighborAO = texture(u_currentAO, neighborUV);
                vec4 diff = neighborAO - neighborMean;
                neighborStdDev += diff * diff;
            }
        }
        neighborStdDev = sqrt(neighborStdDev / 9.0);

        // Clamp history to neighborhood statistics
        vec4 boxMin = neighborMean - neighborStdDev * 4.0;
        vec4 boxMax = neighborMean + neighborStdDev * 4.0;
        vec4 clampedHistory = clamp(historyAO, boxMin, boxMax);

        // ============================================================
        // Depth disocclusion check
        // ============================================================
        float currentDepth = texture(u_depthTexture, uv).r;
        vec3 currentPos = reconstructViewPos(uv, currentDepth);

        // Estimate history depth from reprojection
        float historyDepth = texture(u_depthTexture, historyUV).r;
        vec3 historyPos = reconstructViewPos(historyUV, historyDepth);

        // Check depth consistency
        float depthDiff = abs(currentPos.z - historyPos.z);
        float depthThreshold = abs(currentPos.z) * 0.01; // 1% tolerance

        bool depthValid = depthDiff < depthThreshold;

        // ============================================================
        // Adaptive blend based on confidence
        // ============================================================
        float blendFactor = u_temporalAlpha;

        // Increase blend (more current) if history seems invalid
        if (!depthValid) {
            blendFactor = 0.5; // More aggressive rejection
        }

        // Lower blend (more history) for stable regions
        // Check if current and clamped history are similar
        float historyDiff = length(currentAO - clampedHistory);
        if (historyDiff < 0.05) {
            blendFactor = min(blendFactor, 0.05); // Very stable, use more history
        }

        // Exponential moving average
        finalAO = mix(clampedHistory, currentAO, blendFactor);
        float clampAmount = length(historyAO - clampedHistory);
        if (clampAmount > 0.1) {
            debugColor = vec4(1.0, 1.0, 0.0, 1.0); // Yellow: Clamped by neighborhood
        }
    } else {
        // No valid history - use current frame only
        finalAO = currentAO;
        debugColor = vec4(1.0, 0.0, 0.0, 1.0); // Red: Reprojected out of bounds
    }

    // Ensure valid range
    finalAO = clamp(finalAO, 0.0, 1.0);

    imageStore(u_outAO, pixelCoord, finalAO);
}