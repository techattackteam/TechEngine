#version 460

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0) uniform sampler2D u_depthMap;

uniform mat4 u_previousViewProjection;
uniform mat4 u_currentViewProjection;
uniform mat4 u_currentViewProjectionInverse;
uniform ivec2 u_screenSize;

layout (binding = 0, rg16f) uniform writeonly image2D u_motionMap;

vec3 reconstructViewPosition(vec2 uv, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clip = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 view = u_currentViewProjectionInverse * clip;
    return view.xyz / view.w;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);

    if (pixelCoord.x >= u_screenSize.x || pixelCoord.y >= u_screenSize.y) {
        return;
    }

    vec2 uv = (vec2(pixelCoord) + vec2(0.5)) / vec2(u_screenSize);
    float depth = texture(u_depthMap, uv).r;

    if (depth >= 0.99999) {
        imageStore(u_motionMap, pixelCoord, vec4(0.0));
        return;
    }

    vec3 viewPosition = reconstructViewPosition(uv, depth);

    vec4 currentClipPos = u_currentViewProjection * vec4(viewPosition, 1.0);
    vec3 currentNDC = currentClipPos.xyz / currentClipPos.w;


    vec4 previousClipPos = u_previousViewProjection * vec4(viewPosition, 1.0);
    vec3 previousNDC = previousClipPos.xyz / previousClipPos.w;

    vec2 currentUV = currentNDC.xy * 0.5 + 0.5;
    vec2 previousUV = previousNDC.xy * 0.5 + 0.5;

    vec2 motionVector = previousUV - currentUV;

    imageStore(u_motionMap, pixelCoord, vec4(motionVector, 0.0, 0.0));
}