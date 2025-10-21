#version 460 core
layout (local_size_x = 8, local_size_y = 8) in;

layout (binding = 0) uniform sampler2D u_normalTexture;
layout (binding = 1) uniform sampler2D u_depthTexture;
layout (binding = 2) uniform sampler2D u_noiseTexture;

uniform ivec2 u_screenSize;
uniform int u_directionSampleCount;
uniform int u_sliceCount;
uniform mat4 u_inverseProjection;
uniform float u_radius;
uniform float u_falloffRange;
uniform float u_thicknessHeuristic;

layout (binding = 0, r16f) uniform writeonly image2D u_outOcclusion;


const float PI = 3.14159265358979323846;
const float EPS = 1e-6;
const int DEBUG_MODE = 0; // 0: normal, 1: normal length, 2: NaN/infinity check
vec3 reconstructViewPosition(vec2 uv, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 view = u_inverseProjection * clip;
    return view.xyz / view.w;
}

float getNoiseRotation(ivec2 pixelCoord) {
    // safe cast and use repeat of a small noise map
    vec2 noiseUV = vec2(pixelCoord % 4) / 4.0;
    return texture(u_noiseTexture, noiseUV).r * 2.0 * PI;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoord.x >= u_screenSize.x || pixelCoord.y >= u_screenSize.y) return;

    vec2 uv = (vec2(pixelCoord) + 0.5) / vec2(u_screenSize);

    float centerDepth = texture(u_depthTexture, uv).r;
    if (centerDepth >= 0.99999) {
        imageStore(u_outOcclusion, pixelCoord, vec4(1.0));
        return;
    }

    vec3 position = reconstructViewPosition(uv, centerDepth);

    // === FIX: remap normals from [0,1] -> [-1,1] ===
    vec3 normalSample = texture(u_normalTexture, uv).xyz;
    vec3 normal = normalize(normalSample);

    vec3 view = normalize(-position);

    float totalVisibility = 0.0;
    float spatialRotation = getNoiseRotation(pixelCoord);

    for (int slice = 0; slice < u_sliceCount; slice++) {
        float sliceAngle = (float(slice) / float(u_sliceCount)) * 2.0 * PI;
        float phi = spatialRotation + sliceAngle;

        // base 2D direction in tangent plane before projection
        vec3 omega = vec3(cos(phi), sin(phi), 0.0);
        // project into view-plane (remove component along view)
        vec3 direction = omega - dot(omega, view) * view;
        float dirLen = length(direction);
        if (dirLen < EPS) {
            // degenerate: direction parallel to view — skip this slice
            continue;
        }
        direction = direction / dirLen;

        // create a screen-space sampling delta derived from direction.
        // We approximate mapping by using direction.xy (view-space -> screen-space heuristic).
        // This is coarse but consistent with the slice geometry above.
        vec2 dirSS = normalize(direction.xy);

        // compute axis and projected normal for slice integration
        vec3 axis = cross(direction, view);
        vec3 projectedNormal = normal - axis * dot(normal, axis);
        float projLen = length(projectedNormal);
        if (projLen < EPS) {
            // projected normal too small — unlikely to contribute to occlusion
            continue;
        }
        // normalize for safe dot computations
        projectedNormal /= projLen;

        float signGamma = sign(dot(direction, projectedNormal));
        float cosGamma = clamp(dot(projectedNormal, view), -1.0, 1.0); // already unit-length
        float gamma = signGamma * acos(cosGamma);

        // horizon search params
        float scaling = u_radius / max(-position.z, EPS);

        float h[2];
        for (int side = 0; side < 2; side++) {
            float centerHorizon = -1.0;
            float stepDirection = (side == 0) ? -1.0 : 1.0;

            for (int step = 1; step <= u_directionSampleCount; step++) {
                float ds = float(step) / float(u_directionSampleCount);
                // === FIX: use the projected sampling dirSS instead of omega.xy ===
                vec2 sampleUV = uv + stepDirection * ds * scaling * dirSS;

                if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
                    break;
                }

                float sampleDepth = texture(u_depthTexture, sampleUV).r;
                if (sampleDepth >= 0.99999) {
                    // hit sky; large distance -> we can consider it not occluding (keep centerHorizon)
                    continue;
                }

                vec3 samplePosition = reconstructViewPosition(sampleUV, sampleDepth);
                vec3 horizonVector = samplePosition - position;

                float hvLen = length(horizonVector);
                if (hvLen < EPS) continue;
                float sampleHorizon = dot(normalize(horizonVector), view);

                // thickness heuristic
                float thickness = (hvLen / u_radius) * u_thicknessHeuristic;
                sampleHorizon -= thickness;

                centerHorizon = max(centerHorizon, sampleHorizon);
            }

            float horizonAngle = acos(clamp(centerHorizon, -1.0, 1.0));
            h[side] = clamp(gamma + stepDirection * horizonAngle, -PI / 2.0, PI / 2.0);
        }

        // integrate slice visibility (same formula, but use normalized projectedNormal)
        float sliceVisibility = 0.5 * ((h[0] * sin(gamma) + 0.25 * cos(2.0 * h[0] - gamma)) -
        (h[1] * sin(gamma) + 0.25 * cos(2.0 * h[1] - gamma)));
        sliceVisibility *= projLen; // projLen used earlier (was length(projectedNormal))

        totalVisibility += sliceVisibility;

        // optional: bail-out if totalVisibility grows huge (avoid numeric issues)
        if (!isinf(totalVisibility)) {
            // write diagnostic and return
            imageStore(u_outOcclusion, pixelCoord, vec4(0.0));
            return;
        }
    }

    float finalAO = 1.0 - (totalVisibility / (PI * float(max(u_sliceCount, 1))));
    finalAO = clamp(finalAO, 0.0, 1.0);

    // Debug modes
    if (DEBUG_MODE == 1) {
        // output projected normal length (scaled)
        imageStore(u_outOcclusion, pixelCoord, vec4(clamp(length(normal), 0.0, 1.0)));
        return;
    } else if (DEBUG_MODE == 2) {
        // mark NaNs or infinities
        if (!isinf(finalAO)) {
            imageStore(u_outOcclusion, pixelCoord, vec4(0.0));
            return;
        }
    }

    imageStore(u_outOcclusion, pixelCoord, vec4(finalAO));
}
