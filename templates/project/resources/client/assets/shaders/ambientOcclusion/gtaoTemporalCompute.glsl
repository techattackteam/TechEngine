#version 460

layout (local_size_x = 8, local_size_y = 8) in;

layout (binding = 0) uniform sampler2D u_currentAOHalf;
layout (binding = 1) uniform sampler2D u_historyAOHalf;
layout (binding = 2) uniform sampler2D u_motionVectors;
layout (binding = 3) uniform sampler2D u_depthTexture;
layout (binding = 4) uniform sampler2D u_normalTexture;

uniform ivec2 u_screenSize;
uniform ivec2 u_halfScreenSize;
uniform float u_historyWeight;
uniform float u_depthThreshold;
uniform float u_velocityThreshold;
uniform float u_rotationAngle;
uniform mat4 u_inverseProjection;
uniform float u_radius;

layout (binding = 0, r32f) uniform image2D u_outputAOHalf;

const int NUM_STEPS = 12;        // horizon sampling steps
const float STEP_SIZE = 1.0 / float(NUM_STEPS);
const float PI = 3.14159265359;

vec2 getBaseDirection(ivec2 pixelCoords) {
    int idx = ((pixelCoords.x & 3) + ((pixelCoords.y & 3) << 2));
    float angle = float(idx) * (PI / 8.0);
    return vec2(cos(angle), sin(angle));
}

vec3 reconstructViewPosition(vec2 uv, float depth, mat4 invProj) {
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 view = invProj * clip;
    return view.xyz / view.w;
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoords.x >= u_halfScreenSize.x || pixelCoords.y >= u_halfScreenSize.y) {
        return;
    }

    vec2 texelHalf = vec2(pixelCoords) + vec2(0.5);
    vec2 uvHalf = texelHalf / vec2(u_halfScreenSize);

    vec2 uvFull = (texelHalf * 2.0 - 0.5) / vec2(u_screenSize);

    float currentAO = texture(u_currentAOHalf, uvHalf).r;

    vec2 motionVector = texture(u_motionVectors, uvFull).rg;
    float motionVectorLength = length(motionVector);

    if (motionVectorLength > u_velocityThreshold) {
        imageStore(u_outputAOHalf, pixelCoords, vec4(currentAO, 0.0, 0.0, 0.0));
        return;
    }

    vec2 prevUVHalf = uvHalf - (motionVector / vec2(u_screenSize)) * 0.5;

    bool validHistory = true;
    if (prevUVHalf.x < 0.0 || prevUVHalf.x > 1.0 || prevUVHalf.y < 0.0 || prevUVHalf.y > 1.0) {
        validHistory = false;
    }

    float historyAO = currentAO;
    if (validHistory) {
        vec2 prevUVHalfClamped = clamp(prevUVHalf, vec2(0.0), vec2(1.0));
        historyAO = texelFetch(u_historyAOHalf, ivec2(prevUVHalfClamped * vec2(u_halfScreenSize)), 0).r;

        vec2 prevUVFull = uvFull - (motionVector / vec2(u_screenSize));
        prevUVFull = clamp(prevUVFull, vec2(0.0), vec2(1.0));
        float prevDepth = texture(u_depthTexture, prevUVFull).r;
        float currDepth = texture(u_depthTexture, uvFull).r;
        if (abs(prevDepth - currDepth) > u_depthThreshold) {
            validHistory = false;
        }
    }

    float aoCurr = currentAO;


    float outAO = aoCurr;
    if (validHistory) {
        // Exponential blend with history (paper uses EMA); choose mix here
        outAO = mix(aoCurr, historyAO, u_historyWeight);
    }
    imageStore(u_outputAOHalf, pixelCoords, vec4(outAO, 0.0, 0.0, 0.0));
}