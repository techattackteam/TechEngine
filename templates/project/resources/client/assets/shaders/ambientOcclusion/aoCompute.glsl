#version 460

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0) uniform sampler2D u_depthTexture;
layout (binding = 1) uniform sampler2D u_normalTexture;
//layout (binding = 2) uniform sampler2D u_lightTexture; // For future implementation

uniform bool u_enabled;
uniform float u_directionCount;
uniform float u_stepsPerDirection;
uniform float u_radius;
uniform float u_thickness;
uniform vec2 u_screenSize;
uniform mat4 u_projection;
uniform mat4 u_inverseProjection;

layout (binding = 0, rgba16f) uniform writeonly image2D u_outAO;

const float PI = 3.14159265359;
const uint sectorCount = 32u;

vec3 reconstructViewPosition(vec2 uv, float depth) {
    vec4 clip = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 view = u_inverseProjection * clip;
    return view.xyz / view.w;
}

float randf(vec2 coordinates) {
    return mod(52.9829189 * mod(0.06711056 * coordinates.x + 0.00583715 * coordinates.y, 1.0), 1.0);
}

uint getSampleBitmask(float minHorizon, float maxHorizon) {
    uint startBit = uint(minHorizon * float(sectorCount));
    uint horizonAngle = uint(ceil((maxHorizon - minHorizon) * float(sectorCount)));
    uint angleBit = horizonAngle > 0u ? uint(0xFFFFFFFFu >> (sectorCount - horizonAngle)) : 0u;
    uint currentBitfield = angleBit << startBit;
    return 0u | currentBitfield;
}

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoords.x >= int(u_screenSize.x) || pixelCoords.y >= int(u_screenSize.y)) {
        return;
    }

    if (!u_enabled) {
        imageStore(u_outAO, pixelCoords, vec4(vec3(1.0f), 1.0f));
        return;
    }

    vec2 uv = (vec2(pixelCoords) + 0.5) / u_screenSize;

    float depth = texture(u_depthTexture, uv).r;
    vec3 position = reconstructViewPosition(uv, depth);
    vec3 normal = normalize(texture(u_normalTexture, uv).rgb);

    vec3 viewDir = normalize(-position);
    vec2 aspectRatio = vec2(u_screenSize.x / u_screenSize.y, 1.0);
    float jitter = randf(vec2(pixelCoords));
    jitter = 0.0f; // disable jitter for now
    float radius = (-u_radius * u_projection[0][0]) / position.z;

    float occlusion = 0.0;
    vec3 totalGI = vec3(0.0); // accumulated indirect light, for future implementation

    for (float i = 0.0; i < u_directionCount; i++) {
        float phi = (2.0 * PI / u_directionCount) * (i + jitter);
        vec3 direction = vec3(cos(phi), sin(phi), 0.0);

        vec2 correctedDirection = direction.xy / aspectRatio;
        correctedDirection = normalize(correctedDirection);
        direction = vec3(correctedDirection, 0.0);

        vec3 orthoDirection = direction - viewDir * dot(direction, viewDir);
        vec3 axis = cross(direction, viewDir);
        vec3 projectedNormal = normal - axis * dot(normal, axis);
        float projectedLength = length(projectedNormal);
        if (projectedLength < 1e-5) {
            continue;
        }
        float signN = sign(dot(orthoDirection, projectedNormal));
        float consN = clamp(dot(projectedNormal, viewDir) / projectedLength, 0.0, 1.0);
        float n_offset = signN * acos(consN);

        uint bitmask_i = 0u;

        for (int j = 0; j < u_stepsPerDirection; j++) {
            float sampleStep = (j + jitter) / u_stepsPerDirection;
            sampleStep = pow(sampleStep, 2.0);
            float smapleDistance = sampleStep + 0.01f;
            vec2 sampleUV = uv - smapleDistance * radius * direction.xy * aspectRatio;
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
                continue;
            }
            float sampleDepth = texture(u_depthTexture, sampleUV).r; // View space position at step j
            vec3 samplePosition = reconstructViewPosition(sampleUV, sampleDepth);
            vec3 sampleDistance = samplePosition - position;
            float sampleLength = length(sampleDistance);

            if (sampleLength > u_radius) {
                continue;
            }

            vec3 sampleHorizon = sampleDistance / sampleLength;

            float frontHorizon = dot(sampleHorizon, viewDir);
            float backHorizon = dot(normalize((samplePosition - sampleHorizon * u_thickness) - position), viewDir);

            vec2 frontBackHorizon = acos(clamp(vec2(frontHorizon, backHorizon), -1.0, 1.0));

            frontBackHorizon = clamp((frontBackHorizon + n_offset + (PI / 2.0)) / PI, 0.0, 1.0);

            uint bitmask_j = getSampleBitmask(frontBackHorizon.x, frontBackHorizon.y);
            bitmask_i |= bitmask_j;
        }
        occlusion += 1.0 - float(bitCount(bitmask_i)) / float(sectorCount);
    }

    //vec3 finalColor = totalGI / u_directionCount;
    float finalAO = occlusion / u_directionCount;

    imageStore(u_outAO, pixelCoords, vec4(vec3(1.0f), finalAO));
}