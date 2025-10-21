#version 460

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0) uniform sampler2D u_halfResAO;
layout (binding = 1) uniform sampler2D u_fullResDepth;
layout (binding = 2) uniform sampler2D u_fullResNormal;

uniform ivec2 u_screenSize;
uniform mat4 u_inverseProjection;
uniform float u_depthThreshold;

layout (binding = 0, rgba16f) uniform writeonly image2D u_outFullResAO;

vec3 reconstructViewPosition(vec2 uv, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 view = u_inverseProjection * clip;
    return view.xyz / view.w;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);

    if (pixelCoord.x >= u_screenSize.x || pixelCoord.y >= u_screenSize.y) {
        return;
    }

    vec2 uv = (vec2(pixelCoord) + 0.5) / vec2(u_screenSize);

    float centerDepth = texture(u_fullResDepth, uv).r;
    vec3 centerPos = reconstructViewPosition(uv, centerDepth);
    vec3 centerNormal = normalize(texture(u_fullResNormal, uv).rgb);
    float centerZ = centerPos.z;

    if (centerDepth >= 0.9999) {
        imageStore(u_outFullResAO, pixelCoord, vec4(1.0));
        return;
    }

    vec4 aoSum = vec4(0.0);
    float weightSum = 0.0;

    vec2 halfResCoord = vec2(pixelCoord) / 2.0;
    ivec2 halfResBase = ivec2(floor(halfResCoord));

    for (int y = -1; y < 3; y++) {
        for (int x = -1; x < 3; x++) {
            ivec2 halfResPixel = halfResBase + ivec2(x, y);

            // Convert back to UV space for sampling half-res texture
            vec2 halfResSize = vec2(u_screenSize) / 2.0;
            vec2 halfResUV = (vec2(halfResPixel) + 0.5) / halfResSize;

            // Clamp to valid range
            if (halfResUV.x < 0.0 || halfResUV.x > 1.0 ||
            halfResUV.y < 0.0 || halfResUV.y > 1.0) {
                continue;
            }

            // Sample AO from half-res texture
            vec4 aoSample = texture(u_halfResAO, halfResUV);

            // Now we need to compare against full-res depth at the corresponding position
            // The half-res pixel at (hx, hy) corresponds to full-res pixel at (hx*2, hy*2)
            ivec2 correspondingFullResPixel = halfResPixel * 2;
            vec2 correspondingFullResUV = (vec2(correspondingFullResPixel) + 0.5) / vec2(u_screenSize);

            // Sample full-res depth and normal at the corresponding position
            float sampleDepth = texture(u_fullResDepth, correspondingFullResUV).r;

            // Skip invalid samples
            if (sampleDepth >= 0.9999) {
                continue;
            }

            vec3 samplePos = reconstructViewPosition(correspondingFullResUV, sampleDepth);
            vec3 sampleNormal = normalize(texture(u_fullResNormal, correspondingFullResUV).xyz);

            // ============================================================
            // Bilateral weights
            // ============================================================

            // 1. Depth similarity (in view space Z)
            float depthDiff = abs(samplePos.z - centerZ);
            float depthWeight = exp(-depthDiff / u_depthThreshold);

            // 2. Normal similarity (cosine)
            float normalDot = dot(centerNormal, sampleNormal);
            float normalWeight = pow(max(normalDot, 0.0), 4.0);

            // 3. Spatial distance (in half-res pixels)
            vec2 spatialDist = halfResCoord - vec2(halfResPixel);
            float spatialWeight = exp(-dot(spatialDist, spatialDist) / 2.0);

            // Combined weight
            float weight = depthWeight * normalWeight * spatialWeight;

            aoSum += aoSample * weight;
            weightSum += weight;
        }
    }

    ivec2 correspondingFullResPixel = halfResBase * 2;
    vec2 correspondingFullResUV = (vec2(correspondingFullResPixel) + 0.5) / vec2(u_screenSize);



    vec3 sampleNormal = normalize(texture(u_fullResNormal, correspondingFullResUV).xyz);
    vec4 finalAO = aoSum / max(weightSum, 0.0001);
    finalAO = clamp(finalAO, 0.0, 1.0);

    imageStore(u_outFullResAO, pixelCoord, finalAO);
}