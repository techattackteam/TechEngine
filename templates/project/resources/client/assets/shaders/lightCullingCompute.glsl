#version 460

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

struct Light {
    vec3 position;
    int type;

    vec3 direction;
    float radius;

    vec3 color;
    float intensity;

    float innerCutoff;
    float outerCutoff;
    int castShadow;
    float pad2;

    uvec2 shadowHandle[4];

    mat4 lightSpaceMatrix[4];

    float cascadeSplits[4];
};

// Info for each tile
struct TileInfo {
    uint offset;
    uint lightsCount;
};

layout (std430, binding = 0) buffer LightBuffer {
    Light lights[];
};

// --- Output Buffers ---
// A tightly packed list of light indices
layout (std430, binding = 1) buffer LightIndexBuffer {
    uint lightIndices[];
};

layout (std430, binding = 2) buffer TileInfoBuffer {
    TileInfo tiles[];
};

// Global atomic counter to manage writing to the LightIndexBuffer
layout (std430, binding = 3) buffer AtomicCounter {
    uint nextLightIndexOffset;
};

// Depth texture
uniform sampler2D u_depthMap;

// Camera matrices
uniform mat4 u_view;
uniform mat4 u_inverseProjection; // Pre-calculate inverse on CPU for efficiency

// Screen dimensions
uniform ivec2 u_screenSize;

// This is a small, fast "scratchpad" of memory private to each workgroup.
shared uint s_visibleLightIndices[1024]; // Max 1024 lights per tile
shared uint s_visibleLightCount = 0;
shared uint s_minDepthInt;
shared uint s_maxDepthInt;

struct Plane {
    vec3 normal;
    float distance; // Distance from origin
};

const int TILE_SIZE = 16;

vec3 screenToView(vec2 screenPos, float depth) {
    vec4 clipSpacePos = vec4(screenPos * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePos = u_inverseProjection * clipSpacePos;
    return viewSpacePos.xyz / viewSpacePos.w;
}

bool coneIntersectsAABB(vec3 coneApex, vec3 coneDir, float cosConeAngle, vec3 aabbMin, vec3 aabbMax) {
    // Basic test: Is the apex inside the AABB?
    if (all(greaterThanEqual(coneApex, aabbMin)) && all(lessThanEqual(coneApex, aabbMax))) {
        return true;
    }

    // Find the closest point on the AABB to the cone's apex
    vec3 closestPoint = clamp(coneApex, aabbMin, aabbMax);
    vec3 v = closestPoint - coneApex;
    float v_len_sq = dot(v, v);
    float v_dot_d = dot(v, coneDir);

    // Test if the closest point is inside the cone
    if (v_dot_d > 0.0 && (v_dot_d * v_dot_d) > (v_len_sq * cosConeAngle * cosConeAngle)) {
        return true;
    }

    return false;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    int threadIndex = int(gl_LocalInvocationIndex);
    ivec2 tileCoord = ivec2(gl_WorkGroupID.xy);

    int tilesX = (u_screenSize.x + TILE_SIZE - 1) / TILE_SIZE;
    int tileIndex = tileCoord.y * tilesX + tileCoord.x;

    // Initialize shared memory
    if (threadIndex == 0) {
        s_visibleLightCount = 0;
        s_minDepthInt = 0xFFFFFFFF; // Max uint value (far plane)
        s_maxDepthInt = 0;          // Min uint value (near plane)
    }

    if (pixelCoord.x >= u_screenSize.x || pixelCoord.y >= u_screenSize.y) {
        barrier();
        return;
    }

    barrier();

    vec2 uv = (vec2(pixelCoord) + 0.5) / vec2(u_screenSize);
    uv = clamp(uv, vec2(0.0), vec2(1.0));
    float depth = texture(u_depthMap, uv).r;

    if (depth < 1.0) {
        uint depthInt = floatBitsToUint(depth);
        atomicMin(s_minDepthInt, depthInt);
        atomicMax(s_maxDepthInt, depthInt);
    }
    barrier();

    if (threadIndex == 0) {
        if (s_minDepthInt == 0xFFFFFFFF) {
            s_visibleLightCount = 0;
        } else {
            float minDepth = uintBitsToFloat(s_minDepthInt);
            float maxDepth = uintBitsToFloat(s_maxDepthInt);

            vec2 tileUVMin = (vec2(tileCoord) * TILE_SIZE) / u_screenSize;
            vec2 tileUVMax = (vec2(tileCoord + ivec2(1)) * TILE_SIZE) / u_screenSize;

            tileUVMin = clamp(tileUVMin, vec2(0.0), vec2(1.0));
            tileUVMax = clamp(tileUVMax, vec2(0.0), vec2(1.0));


            vec3 corners[8];
            corners[0] = screenToView(vec2(tileUVMin.x, tileUVMin.y), minDepth);
            corners[1] = screenToView(vec2(tileUVMax.x, tileUVMin.y), minDepth);
            corners[2] = screenToView(vec2(tileUVMin.x, tileUVMax.y), minDepth);
            corners[3] = screenToView(vec2(tileUVMax.x, tileUVMax.y), minDepth);
            corners[4] = screenToView(vec2(tileUVMin.x, tileUVMin.y), maxDepth);
            corners[5] = screenToView(vec2(tileUVMax.x, tileUVMin.y), maxDepth);
            corners[6] = screenToView(vec2(tileUVMin.x, tileUVMax.y), maxDepth);
            corners[7] = screenToView(vec2(tileUVMax.x, tileUVMax.y), maxDepth);

            // Compute AABB bounds
            vec3 aabbMin = corners[0];
            vec3 aabbMax = corners[0];
            for (int i = 1; i < 8; i++) {
                aabbMin = min(aabbMin, corners[i]);
                aabbMax = max(aabbMax, corners[i]);
            }

            vec3 bottomLeft = corners[0];
            vec3 bottomRight = corners[1];
            vec3 topLeft = corners[2];
            vec3 topRight = corners[3];

            Plane frustumPlanes[4];
            frustumPlanes[0] = Plane(normalize(cross(topLeft, bottomLeft)), 0.0);       // Left
            frustumPlanes[1] = Plane(normalize(cross(bottomRight, topRight)), 0.0);     // Right
            frustumPlanes[2] = Plane(normalize(cross(topRight, topLeft)), 0.0);         // Top
            frustumPlanes[3] = Plane(normalize(cross(bottomLeft, bottomRight)), 0.0);   // Bottom

            // Cull the lights against the frustum
            uint lightCount = lights.length();
            for (uint lightIndex = 0; lightIndex < lightCount; lightIndex++) {
                Light light = lights[lightIndex];
                vec4 lightViewPos = u_view * vec4(light.position, 1.0);
                bool insideFrustum = true;

                if (light.type == 1) {
                    if (s_visibleLightCount < 1024) {
                        s_visibleLightIndices[s_visibleLightCount] = lightIndex;
                        s_visibleLightCount++;
                    }
                } else if (light.type == 0) {
                    float radius = light.radius;
                    for (int p = 0; p < 4; p++) {
                        float distance = dot(frustumPlanes[p].normal, lightViewPos.xyz);
                        if (distance < -radius) {
                            insideFrustum = false;
                            break;
                        }
                    }

                    if (!insideFrustum) {
                        continue;
                    }

                    if (lightViewPos.z + radius < aabbMin.z || lightViewPos.z - radius > aabbMax.z) {
                        continue;
                    }


                } else if (light.type == 2) {
                    vec3 lightViewDir = normalize((u_view * vec4(light.direction, 0.0)).xyz);
                    float cosOuterAngle = light.outerCutoff;

                    // A simple, conservative test: check if the AABB's center is within a slightly expanded cone.
                    // This is not perfect but is very fast and rejects most obvious cases.
                    //vec3 centerToLight = normalize(lightViewPos.xyz - (aabbMin + aabbMax) * 0.5);
                    //float aabbAngularRadius = length(aabbMax - aabbMin) / (2.0 * length(lightViewPos.xyz - (aabbMin + aabbMax) * 0.5));

                    //if (dot(centerToLight, -lightViewDir) <= cosOuterAngle - aabbAngularRadius) {
                    //    continue; // Tile is outside the spotlight's cone
                    //}
                    //if (!coneIntersectsAABB(lightViewPos.xyz, -lightViewDir, cosOuterAngle, aabbMin, aabbMax)) {
                    //    continue;
                    //}
                    //TODO: Implement better spotlight culling
                }

                if (insideFrustum && s_visibleLightCount < 1024) {
                    s_visibleLightIndices[s_visibleLightCount] = lightIndex;
                    s_visibleLightCount++;
                }
            }
        }
    }
    barrier();

    if (threadIndex == 0) {
        uint offset = atomicAdd(nextLightIndexOffset, s_visibleLightCount);
        tiles[tileIndex].offset = offset;
        tiles[tileIndex].lightsCount = s_visibleLightCount;
    }
    barrier();
    uint offset = tiles[tileIndex].offset;
    uint count = tiles[tileIndex].lightsCount;

    for (uint i = threadIndex; i < count; i += 256) {
        lightIndices[offset + i] = s_visibleLightIndices[i];
    }
}