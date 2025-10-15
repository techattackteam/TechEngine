#version 460

in vec2 v_texCoord;

uniform sampler2D u_depthTexture;
uniform sampler2D u_noiseTexture;

uniform mat4 u_projection;
uniform mat4 u_inverseProjection;
uniform ivec2 u_screenSize;

uniform float u_radius;
uniform float u_strength;

out float fragColor;

#define NUMBER_DIRECTIONS = 6
#define NUMBER_STEPS = 4
#define PI 3.14159265

vec3 reconstructPositionVS(vec2 uv, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = u_inverseProjection * clipSpacePosition;
    return viewSpacePosition.xyz / viewSpacePosition.w;
}

vec3 reconstructNormalVS(vec3 posVS) {
    vec2 texelSize = 1.0 / vec2(u_screenSize);
    float depthRight = texture(u_depthTexture, v_texCoord + vec2(texelSize.x, 0.0)).r;
    float depthLeft = texture(u_depthTexture, v_texCoord - vec2(texelSize.x, 0.0)).r;
    float depthUp = texture(u_depthTexture, v_texCoord + vec2(0.0, texelSize.y)).r;
    float depthDown = texture(u_depthTexture, v_texCoord - vec2(0.0, texelSize.y)).r;

    vec3 posRight = reconstructPositionVS(v_texCoord + vec2(texelSize.x, 0.0), depthRight);
    vec3 posLeft = reconstructPositionVS(v_texCoord - vec2(texelSize.x, 0.0), depthLeft);
    vec3 posUp = reconstructPositionVS(v_texCoord + vec2(0.0, texelSize.y), depthUp);
    vec3 posDown = reconstructPositionVS(v_texCoord - vec2(0.0, texelSize.y), depthDown);

    vec3 dx = posRight - posLeft;
    vec3 dy = posUp - posDown;
    return normalize(cross(dy, dx));
}

void main() {

    float depth = texture(u_depthTexture, v_texCoord).r;

    if (depth >= 1.0) {
        fragColor = 1.0;
        return;
    }

    vec3 posVS = reconstructPositionVS(v_texCoord, depth);
    vec3 normalVS = reconstructNormalVS(posVS);

    vec2 noiseUV = v_texCoord * vec2(u_screenSize) / 4.0;
    vec3 randomVec = normalize(texture(u_noiseTexture, noiseUV).xyz);

    float occlusion = 0.0;

    for (int i = 0; i < NUMBER_DIRECTIONS; i++) {
        float angle = float(i) / (PI * 2.0 * float(NUMBER_DIRECTIONS));

        vec3 directionVS = reflect(vec3(cos(angle), sin(angle), 0.0), randomVec);

        // Ensure the sample direction is in the same hemisphere as the normal
        if (dot(normalVS, directionVS) < 0.0) {
            directionVS = -directionVS;
        }
    }
}