#version 460

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

struct volumeAABB {
    vec4 minPoint; // xyz: position, w: padding
    vec4 maxPoint; // xyz: size, w: padding
};

layout (std430, binding = 0) buffer VolumeAABBBuffer {
    volumeAABB aabbs[];
};

uniform mat4 u_inverseProjection;
uniform uvec2 u_screenSize;
uniform uvec3 u_gridSize;

uniform float u_nearPlane;
uniform float u_farPlane;

vec3 screenToViewPosition(vec2 screenPosition) {
    vec2 texCoord = screenPosition.xy / u_screenSize.xy;
    vec4 clip = vec4(vec2(texCoord.x, texCoord.y) * 2.0 - 1.0, -1.0, 1.0);

    vec4 view = u_inverseProjection * clip;
    view = view / view.w;

    return view. xyz;
}

vec3 lineIntersectZPlane(vec3 point, float zDistance) {
    float t = zDistance / point.z;
    return point * t;
}

float sliceToDepth(float slice) {
    float ratio = u_farPlane / u_nearPlane;
    float exponent = slice / float(u_gridSize.z);
    return u_nearPlane * pow(ratio, exponent);
}

void main() {

    uint tileIndex = gl_WorkGroupID.x + (gl_WorkGroupID.y * u_gridSize.x) + (gl_WorkGroupID.z * u_gridSize.x * u_gridSize.y);
    vec2 tileSize = vec2(u_screenSize) / vec2(u_gridSize.xy);

    vec2 minPointScreen = vec2(gl_WorkGroupID.xy) * tileSize;
    vec2 maxPointScreen = vec2(gl_WorkGroupID.xy + uvec2(1)) * tileSize;

    vec3 maxPointView = screenToViewPosition(maxPointScreen);
    vec3 minPointView = screenToViewPosition(minPointScreen);

    float tileNear = -sliceToDepth(float(gl_WorkGroupID.z));
    float tileFar = -sliceToDepth(float(gl_WorkGroupID.z + 1));

    vec3 minMinNear = lineIntersectZPlane(minPointView, tileNear);
    vec3 maxMinNear = lineIntersectZPlane(vec3(maxPointView.x, minPointView.y, minPointView.z), tileNear);
    vec3 minMaxNear = lineIntersectZPlane(vec3(minPointView.x, maxPointView.y, minPointView.z), tileNear);
    vec3 maxMaxNear = lineIntersectZPlane(maxPointView, tileNear);

    vec3 minMinFar = lineIntersectZPlane(minPointView, tileFar);
    vec3 maxMinFar = lineIntersectZPlane(vec3(maxPointView.x, minPointView.y, minPointView.z), tileFar);
    vec3 minMaxFar = lineIntersectZPlane(vec3(minPointView.x, maxPointView.y, minPointView.z), tileFar);
    vec3 maxMaxFar = lineIntersectZPlane(maxPointView, tileFar);

    vec3 aabbMin = min(
        min(min(minMinNear, maxMinNear), min(minMaxNear, maxMaxNear)),
        min(min(minMinFar, maxMinFar), min(minMaxFar, maxMaxFar))
    );

    vec3 aabbMax = max(
        max(max(minMinNear, maxMinNear), max(minMaxNear, maxMaxNear)),
        max(max(minMinFar, maxMinFar), max(minMaxFar, maxMaxFar))
    );

    aabbs[tileIndex].minPoint = vec4(aabbMin, 0.0);
    aabbs[tileIndex].maxPoint = vec4(aabbMax, 0.0);
}