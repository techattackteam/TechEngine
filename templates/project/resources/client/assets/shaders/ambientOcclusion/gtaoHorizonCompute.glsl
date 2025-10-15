#version 460
layout (binding = 0) uniform sampler2D u_depthFull;    // full-res depth [0..1]
layout (binding = 1) uniform sampler2D u_normalFull;   // full-res normal (optional, packed [0..1], if not present, set to black tex)
layout (binding = 2) uniform sampler2D u_noise;        // small tiled noise (e.g. 32x32)

uniform mat4 u_inverseProjection;
uniform ivec2 u_fullSize;
uniform ivec2 u_halfSize;
uniform int u_searchSteps = 32;
uniform float u_stepSize = 1.0;
uniform float u_radiusPixels = 64.0;
uniform float u_thicknessScale = 0.05;
uniform float u_frameRotation = 0.0;
uniform int u_frameIndex = 0;
uniform int u_numDirections = 8;

layout (binding = 0, r32f) uniform writeonly image2D u_aoHalf;
layout (binding = 1, rgba16f) uniform writeonly image2D u_bentNormal;


const float PI = 3.14159265358979323846;


vec3 viewPosFromDepth(vec2 uv, float depth, mat4 invProj) {
    float z = depth * 2.0 - 1.0; // NDC z
    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 view = invProj * clip;
    return view.xyz / view.w;
}

vec3 normalFromDepth(sampler2D depthTex, vec2 uv, ivec2 fullSize, mat4 invProj) {
    vec2 ts = 1.0 / vec2(fullSize);
    float dC = texture(depthTex, uv).r;
    float dR = texture(depthTex, uv + vec2(ts.x, 0.0)).r;
    float dU = texture(depthTex, uv + vec2(0.0, ts.y)).r;
    vec3 pC = viewPosFromDepth(uv, dC, invProj);
    vec3 pR = viewPosFromDepth(uv + vec2(ts.x, 0.0), dR, invProj);
    vec3 pU = viewPosFromDepth(uv + vec2(0.0, ts.y), dU, invProj);
    return normalize(cross(pR - pC, pU - pC));
}

vec2 rotateDir(vec2 d, float angle) {
    float c = cos(angle), s = sin(angle);
    return vec2(c * d.x - s * d.y, s * d.x + c * d.y);
}


float analyticSlice(float theta1, float theta2, float gamma) {
    // theta1, theta2 in radians, gamma is angle between view dir and normal
    float part1 = -cos(2.0 * theta1 - gamma) - cos(gamma) + 2.0 * theta1 * sin(gamma);
    float part2 = -cos(2.0 * theta2 - gamma) + cos(gamma) + 2.0 * theta2 * sin(gamma);
    return 0.25 * (part1 + part2);
}

float maxHorizonAngle(sampler2D depthTex, vec2 uvFull, vec2 dirPix, int steps, float stepSize,
                      mat4 invProj, ivec2 fullSize, float thicknessScale, float radiusPixels) {
    float dC = texture(depthTex, uvFull).r;
    vec3 pC = viewPosFromDepth(uvFull, dC, invProj);

    float maxAngle = -3.14159; // sentinel low
    // clamp steps by radius
    float maxSteps = min(float(steps), radiusPixels / max(1.0, stepSize));

    for (int i = 1; i <= int(maxSteps); ++i) {
        float offsetPx = float(i) * stepSize;
        vec2 offsetUV = (dirPix * offsetPx) / vec2(fullSize);
        vec2 sampleUV = uvFull + offsetUV;

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
            break;
        }

        float dS = texture(depthTex, sampleUV).r;

        if (dS >= 0.99999)
        continue; // skip sky

        vec3 pS = viewPosFromDepth(sampleUV, dS, invProj);
        vec3 v = pS - pC;
        float horizDist = length(v.xy);
        if (horizDist <= 1e-6) continue;
        float angle = atan(v.z, horizDist);

        float thickness = thicknessScale * -pC.z;
        if (length(v) < thickness) continue;

        maxAngle = max(maxAngle, angle);
    }
    return maxAngle;
}

layout (local_size_x = 8, local_size_y = 8) in;
void main() {
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
    if (gid.x >= u_halfSize.x || gid.y >= u_halfSize.y) return;

    vec2 texelHalf = vec2(gid) + vec2(0.5);
    vec2 uvHalf = texelHalf / vec2(u_halfSize);

    vec2 uvFull = (texelHalf * 2.0 - 0.5) / vec2(u_fullSize);

    float depthC = texture(u_depthFull, uvFull).r;
    if (depthC >= 0.99999) {
        imageStore(u_aoHalf, gid, vec4(1.0, 0.0, 0.0, 0.0));
        imageStore(u_bentNormal, gid, vec4(0.5, 0.5, 1.0, 0.0));
        return;
    }

    vec3 normal = texture(u_normalFull, uvFull).rgb * 2.0 - 1.0;
    if (length(normal) < 1e-3) {
        normal = normalFromDepth(u_depthFull, uvFull, u_fullSize, u_inverseProjection);
    }

    vec3 viewPos = viewPosFromDepth(uvFull, depthC, u_inverseProjection);
    vec3 viewDir = normalize(-viewPos);
    float gamma = acos(clamp(dot(normal, viewDir), -1.0, 1.0));

    vec2 baseDir = vec2(1.0, 0.0);

    vec2 noiseUV = (vec2(gid) + vec2(u_frameIndex % 32)) / vec2(u_halfSize);
    float n = texture(u_noise, noiseUV).r;
    float pixelRotation = u_frameRotation + (n - 0.5) * (PI / float(u_numDirections)); // small jitter

    int dirIndex = ((gid.x + gid.y) + u_frameIndex) % u_numDirections;
    float angle = pixelRotation + float(dirIndex) * (PI * 2.0 / float(u_numDirections));
    vec2 dirUnit = vec2(cos(angle), sin(angle));

    vec2 dirFullPixels = dirUnit;

    float thetaPos = maxHorizonAngle(u_depthFull, uvFull, dirFullPixels, u_searchSteps, u_stepSize, u_inverseProjection, u_fullSize, u_thicknessScale, u_radiusPixels);
    float thetaNeg = maxHorizonAngle(u_depthFull, uvFull, -dirFullPixels, u_searchSteps, u_stepSize, u_inverseProjection, u_fullSize, u_thicknessScale, u_radiusPixels);

/**
    if (thetaPos < -3.0) thetaPos = -PI * 0.5;
    if (thetaNeg < -3.0) thetaNeg = -PI * 0.5;
*/

    float aHat = analyticSlice(thetaPos, thetaNeg, gamma);
    float sliceVis = clamp(aHat, 0.0, 1.0);

    vec3 bn_unnormalized = vec3(
    dirUnit.x * (thetaPos - thetaNeg),
    dirUnit.y * (thetaPos - thetaNeg),
    cos((thetaPos + thetaNeg) * 0.5)
    );

    vec3 bn;

    if (dot(bn_unnormalized, bn_unnormalized) < 0.0001) {
        bn = vec3(0.0, 0.0, 1.0);
    } else {
        bn = normalize(bn_unnormalized);
    }
    // confidence can be derived from horizon strength; we store 1.0 for now
    float confidence = 1.0;

    imageStore(u_aoHalf, gid, vec4(sliceVis, 0.0, 0.0, 0.0));
    imageStore(u_bentNormal, gid, vec4(bn * 0.5 + 0.5, confidence));
}
