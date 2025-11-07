#version 460

layout (local_size_x = 16, local_size_y = 9, local_size_z = 4) in;

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
struct TileInfo {
    uint offset;
    uint lightsCount;
};

struct volumeAABB {
    vec4 minPoint; // xyz: position, w: padding
    vec4 maxPoint; // xyz: size, w: padding
};

layout (std430, binding = 0) buffer LightBuffer {
    Light lights[];
};

layout (std430, binding = 1) buffer LightIndexBuffer {
    uint lightIndices[];
};

layout (std430, binding = 2) buffer globalIndexCountSSBO {
    uint globalIndexCount;
};

layout (std430, binding = 3) buffer TileInfoBuffer {
    TileInfo tiles[];
};

layout (std430, binding = 4) buffer VolumeAABBBuffer {
    volumeAABB aabbs[];
};

// Depth texture
uniform sampler2D u_depthMap;

// Camera matrices
uniform mat4 u_view;

shared uint s_visibleLightIndices[576]; // Max 1024 lights
shared uint s_visibleLightCount;
/**
vec3 screenToView(vec2 screenPos, float depth) {
    vec4 clipSpacePos = vec4(screenPos * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePos = u_inverseProjection * clipSpacePos;
    return viewSpacePos.xyz / viewSpacePos.w;
}
*/

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

float sqDistPointAABB(vec3 point, uint tile) {
    float sqDist = 0.0;
    volumeAABB currentAABB = aabbs[tile];

    for (int i = 0; i < 3; ++i) {
        float v = point[i];
        if (v < currentAABB.minPoint[i]) {
            sqDist += pow(currentAABB.minPoint[i] - v, 2);
        } else if (v > currentAABB.maxPoint[i]) {
            sqDist += pow(v - currentAABB.maxPoint[i], 2);
        }
    }
    return sqDist;
}

bool testPointLightAABB(uint sharedLightIndex, uint tile) {
    uint lightIndex = s_visibleLightIndices[sharedLightIndex];
    Light light = lights[lightIndex];
    float radius = light.radius;
    vec3 center = (u_view * vec4(light.position, 1.0)).xyz;
    return sqDistPointAABB(center, tile) <= radius * radius;
}

void main() {
    // Initialize global counter only once (first thread of first workgroup)
    if (gl_GlobalInvocationID.x == 0 && gl_GlobalInvocationID.y == 0 && gl_GlobalInvocationID.z == 0) {
        globalIndexCount = 0;
    }

    // Initialize shared memory for this workgroup
    if (gl_LocalInvocationIndex == 0) {
        s_visibleLightCount = 0;
    }

    barrier();
    memoryBarrierBuffer();

    uint threadCount = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;
    uint tileIndex = gl_LocalInvocationIndex + gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z * gl_WorkGroupID.z;

    uint visibleLightCount = 0;
    uint visibleLightIndices[100]; // Max 100 lights per tile
    uint numBatches = (lights.length() + threadCount - 1) / threadCount;

    for (uint batch = 0; batch < numBatches; batch++) {
        uint lightIndex = batch * threadCount + gl_LocalInvocationIndex;

        if (lightIndex < lights.length()) {
            s_visibleLightIndices[gl_LocalInvocationIndex] = lightIndex;
        }
        barrier();

        for (uint light = 0; light < threadCount; ++light) {
            if (testPointLightAABB(light, tileIndex) && visibleLightCount < 100) {
                visibleLightIndices[visibleLightCount] = batch * threadCount + light;
                visibleLightCount += 1;
            }
        }
        barrier();
    }


    uint offset = atomicAdd(globalIndexCount, visibleLightCount);

    for (uint i = 0; i < visibleLightCount; ++i) {
        lightIndices[offset + i] = visibleLightIndices[i];
    }

    tiles[tileIndex].offset = offset;
    tiles[tileIndex].lightsCount = visibleLightCount;

}