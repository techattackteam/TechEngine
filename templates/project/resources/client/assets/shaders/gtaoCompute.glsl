#version 460

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (r8, binding = 0) uniform writeonly image2D u_outputImage;

uniform sampler2D u_depthTexture;
uniform sampler2D u_noiseTexture;

uniform mat4 u_projection;
uniform mat4 u_inverseProjection;
uniform ivec2 u_screenSize;

uniform float u_radius;
uniform float u_strength;

const float PI = 3.14159265359;
const int NUM_DIRECTIONS = 6;
const int NUM_STEPS = 4;

// Reconstruct view-space position from depth
vec3 getViewPosition(vec2 uv, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = u_inverseProjection * clipSpacePosition;
    return viewSpacePosition.xyz / viewSpacePosition.w;
}

// Reconstruct view-space normal from depth buffer
vec3 getViewNormal(vec2 uv, vec3 viewPos) {
    vec2 texelSize = 1.0 / vec2(u_screenSize);

    float depthR = texture(u_depthTexture, uv + vec2(texelSize.x, 0.0)).r;
    float depthL = texture(u_depthTexture, uv - vec2(texelSize.x, 0.0)).r;
    float depthU = texture(u_depthTexture, uv + vec2(0.0, texelSize.y)).r;
    float depthD = texture(u_depthTexture, uv - vec2(0.0, texelSize.y)).r;

    vec3 posR = getViewPosition(uv + vec2(texelSize.x, 0.0), depthR);
    vec3 posL = getViewPosition(uv - vec2(texelSize.x, 0.0), depthL);
    vec3 posU = getViewPosition(uv + vec2(0.0, texelSize.y), depthU);
    vec3 posD = getViewPosition(uv - vec2(0.0, texelSize.y), depthD);

    vec3 dx = posR - posL;
    vec3 dy = posU - posD;

    // Handle edge cases
    if (depthR >= 0.99999 || depthL >= 0.99999) dx = posR - viewPos;
    if (depthU >= 0.99999 || depthD >= 0.99999) dy = posU - viewPos;

    return normalize(cross(dx, dy));
}

// Compute occlusion along a direction
float computeDirectionOcclusion(vec2 uv, vec3 viewPos, vec3 viewNormal, vec2 direction, float noise) {
    vec2 texelSize = 1.0 / vec2(u_screenSize);

    // Scale radius based on view-space depth to maintain world-space consistency
    float screenRadius = u_radius * u_projection[0][0] / -viewPos.z;
    screenRadius = clamp(screenRadius, 2.0, 100.0); // Clamp to reasonable pixel range

    vec2 deltaUV = direction * texelSize * screenRadius / float(NUM_STEPS);

    float maxOcclusion = 0.0;
    vec3 viewDir = normalize(-viewPos);

    for (int i = 1; i <= NUM_STEPS; i++) {
        // Add slight jitter using noise to reduce banding
        float stepOffset = float(i) + noise * 0.5;
        vec2 sampleUV = uv + deltaUV * stepOffset;

        // Bounds check
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
            continue;
        }

        float sampleDepth = texture(u_depthTexture, sampleUV).r;

        // Skip skybox
        if (sampleDepth >= 0.99999) {
            continue;
        }

        vec3 samplePos = getViewPosition(sampleUV, sampleDepth);
        vec3 sampleVec = samplePos - viewPos;
        float sampleDist = length(sampleVec);

        // Skip if too far
        if (sampleDist > u_radius) {
            continue;
        }

        vec3 sampleDir = sampleVec / sampleDist;

        // Calculate angle between sample and view direction (elevation)
        float NdotS = dot(viewNormal, sampleDir);

        // Only consider samples above the tangent plane
        if (NdotS > 0.0) {
            // Distance falloff
            float falloff = 1.0 - (sampleDist / u_radius);
            falloff = falloff * falloff; // Quadratic falloff

            // Thickness heuristic: ignore samples too close (self-occlusion prevention)
            float thickness = 0.01 * -viewPos.z;
            if (sampleDist < thickness) {
                continue;
            }

            // Weight by angle and distance
            float occlusion = NdotS * falloff;
            maxOcclusion = max(maxOcclusion, occlusion);
        }
    }

    return maxOcclusion;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = vec2(pixelCoord) / vec2(u_screenSize);

    float depth = texture(u_depthTexture, uv).r;

    // Skip skybox
    if (depth >= 0.99999) {
        imageStore(u_outputImage, pixelCoord, vec4(1.0));
        return;
    }

    vec3 viewPos = getViewPosition(uv, depth);
    vec3 viewNormal = getViewNormal(uv, viewPos);

    // Get noise for temporal variation
    vec2 noiseUV = vec2(pixelCoord) / 4.0;
    vec2 noiseVec = texture(u_noiseTexture, noiseUV).rg;
    float noiseAngle = noiseVec.r * 2.0 * PI;
    float noiseOffset = noiseVec.g;

    float totalOcclusion = 0.0;

    // Sample in multiple directions around the point
    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        float angle = (float(i) / float(NUM_DIRECTIONS)) * PI + noiseAngle;
        vec2 direction = vec2(cos(angle), sin(angle));

        // Sample in both positive and negative directions
        float occlusionPos = computeDirectionOcclusion(uv, viewPos, viewNormal, direction, noiseOffset);
        float occlusionNeg = computeDirectionOcclusion(uv, viewPos, viewNormal, -direction, noiseOffset);

        // Take the maximum occlusion from both sides
        totalOcclusion += max(occlusionPos, occlusionNeg);
    }

    // Average across all directions
    totalOcclusion /= float(NUM_DIRECTIONS);

    // Apply strength parameter
    totalOcclusion = clamp(totalOcclusion * u_strength, 0.0, 1.0);

    // Convert occlusion to visibility (AO)
    float visibility = 1.0 - totalOcclusion;

    // Optional: fade at far distances
    float distanceFade = smoothstep(40.0, 80.0, -viewPos.z);
    visibility = mix(visibility, 1.0, distanceFade);

    imageStore(u_outputImage, pixelCoord, vec4(visibility));
}