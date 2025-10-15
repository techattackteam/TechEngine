#version 460

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0) uniform sampler2D u_aoHalf;
layout (binding = 1) uniform sampler2D u_depthFull;
layout (binding = 2) uniform sampler2D u_normalFull;

uniform ivec2 u_screenSize;
uniform ivec2 u_halfScreenSize;
uniform float u_depthWeight = 80.0;   // depth sensitivity
uniform float u_normalWeight = 4.0;   // normal similarity weight
uniform int u_halfKernel = 2;

layout (binding = 0, r32f) uniform writeonly image2D u_aoFull;

void main() {
    ivec2 pixelCoords = ivec2(gl_GlobalInvocationID.xy);

    if (pixelCoords.x >= u_screenSize.x || pixelCoords.y >= u_screenSize.y) {
        return;
    }

    vec2 uvFull = (vec2(pixelCoords) + 0.5) / vec2(u_screenSize);
    vec2 uvHalf = uvFull * 0.5; // map to half-res uv

    float centerDepth = texture(u_depthFull, uvFull).r;
    vec3 centerNormal = texture(u_normalFull, uvFull).rgb * 2.0 - 1.0;

    vec2 halfTexel = vec2(u_halfScreenSize);
    vec2 centerTexel = uvHalf * halfTexel;

    float sumW = 0.0;
    float sumAO = 0.0;

    for (int y = -u_halfKernel; y <= u_halfKernel; ++y) {
        for (int x = -u_halfKernel; x <= u_halfKernel; ++x) {
            vec2 offset = vec2(x, y);
            vec2 sampleTexel = centerTexel + offset;
            vec2 sampleUV = sampleTexel / halfTexel;

            // Boundary check
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
                continue;
            }

            float sampleDepth = texture(u_depthFull, sampleUV * 2.0).r;
            vec3 sampleNormal = texture(u_normalFull, sampleUV * 2.0).rgb * 2.0 - 1.0;
            float sampleAO = texture(u_aoHalf, sampleUV).r;

            float depthDiff = abs(sampleDepth - centerDepth);
            float normalDiff = dot(sampleNormal, centerNormal);

            float wDepth = exp(-depthDiff * u_depthWeight);
            float wNormal = exp((normalDiff - 1.0) * u_normalWeight);
            float weight = wDepth * wNormal;

            sumW += weight;
            sumAO += sampleAO * weight;
        }
    }
    float outAO = (sumW > 0.0) ? (sumAO / sumW) : 1.0;
    imageStore(u_aoFull, pixelCoords, vec4(outAO, 0.0, 0.0, 0.0));
}