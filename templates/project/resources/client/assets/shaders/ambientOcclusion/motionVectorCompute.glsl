#version 460

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0) uniform sampler2D u_depthMap;

uniform mat4 u_currentToPreviousMatrix;
uniform ivec2 u_screenSize;

layout (binding = 0, rg16f) uniform writeonly image2D u_motionMap;


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

    float z = depth * 2.0 - 1.0;
    vec2 currentUV = (vec2(pixelCoord) + 0.5) / vec2(u_screenSize);
    vec4 currentClipPos = vec4(currentUV * 2.0 - 1.0, z, 1.0);

    vec4 previousClipPos = u_currentToPreviousMatrix * currentClipPos;

    // 3. Perform perspective divide to get normalized device coordinates [-1, 1]
    previousClipPos.xyz /= previousClipPos.w;

    // 4. Convert the previous position from NDC to UV space [0, 1]
    vec2 previousUV = previousClipPos.xy * 0.5 + 0.5;

    // 5. Calculate the correct motion vector
    vec2 motionVector = currentUV - previousUV;

    imageStore(u_motionMap, pixelCoord, vec4(motionVector, 0.0, 0.0));
}