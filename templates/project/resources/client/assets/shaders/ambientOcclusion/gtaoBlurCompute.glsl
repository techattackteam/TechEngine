#version 460

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0) uniform sampler2D u_aoIn;     // full-res AO
layout (binding = 1) uniform sampler2D u_depthFull; // full depth

uniform ivec2 u_screenSize;
uniform float u_depthWeight = 80.0;
uniform int u_radius = 4;
uniform ivec2 u_axis; // (1,0) for horizontal, (0,1) for vertical

layout (binding = 0, r32f) uniform writeonly image2D u_outputImage;

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);

    if (pixelCoord.x >= u_screenSize.x || pixelCoord.y >= u_screenSize.y) {
        return;
    }

    vec2 uv = (vec2(pixelCoord) + 0.5) / vec2(u_screenSize);
    float centerDepth = texture(u_depthFull, uv).r;
    float sumW = 0.0;
    float sum = 0.0;

    for (int i = -u_radius; i <= u_radius; i++) {
        vec2 sampleUV = uv + (u_axis * i) / vec2(u_screenSize);

        // Bounds check for sample
        if (sampleUV.x < 0 || sampleUV.x >= u_screenSize.x || sampleUV.y < 0 || sampleUV.y >= u_screenSize.y) {
            continue;
        }

        float ao = texture(u_aoIn, sampleUV).r;
        float d = texture(u_depthFull, sampleUV).r;
        float w = exp(-u_depthWeight * abs(d - centerDepth));

        float gw = exp(- float(i * i) / float(2 * u_radius * u_radius));
        float weight = w * gw;
        sumW += weight;
        sum += ao * weight;
    }

    float finalAO = (sumW > 0.0) ? (sum / sumW) : texture(u_aoIn, uv).r;
    imageStore(u_outputImage, pixelCoord, vec4(finalAO));
}