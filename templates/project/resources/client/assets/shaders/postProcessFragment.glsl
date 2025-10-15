#version 460 core

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D u_hdrBuffer;
uniform sampler2D u_bloomBuffer;

uniform float u_bloomStrength = 1.0;

uniform float u_exposure = 1.0;

uniform float u_saturation = 1.2;
uniform float u_contrast = 1.1;
uniform float u_brightness = 1.0;

uniform float u_chromaticAberrationStrength = 1.0;

uniform float u_vignetteStrength = 1.0;
uniform float u_vignettePower = 1.5;


#define EPSILON 0.0001
vec3 reinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 aces_approx(vec3 v) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0, 1.0);
}

vec3 uncharted2(vec3 color) {
    float A = 0.15; // Shoulder Strength
    float B = 0.50; // Linear Strength
    float C = 0.10; // Linear Angle
    float D = 0.20; // Toe Strength
    float E = 0.02; // Toe Numerator
    float F = 0.30; // Toe Denominator
    float W = 11.2; // Linear White Point Value

    color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
    return color / white;
}

vec3 chromaticAberration(sampler2D tex, vec2 uv, float strength) {
    vec2 offset = (uv - 0.5) * 2.0; // -1 to 1
    float dist = length(offset);

    vec2 direction = normalize(offset);
    float aberration = dist * strength * 0.01;

    float r = texture(tex, uv + direction * aberration).r;
    float g = texture(tex, uv).g;
    float b = texture(tex, uv - direction * aberration).b;

    return vec3(r, g, b);
}

float vignette(vec2 uv, float strength, float power) {
    vec2 offset = uv - 0.5;
    float dist = length(offset) * 2.0; // 0 at center, ~1.4 at corners

    float vig = 1.0 - smoothstep(0.0, 1.0, dist);
    vig = pow(vig, power);

    return mix(1.0, vig, strength);
}

void main() {
    //vec3 hdrColor = texture(u_hdrBuffer, texCoords).rgb;
    vec3 hdrColor = chromaticAberration(u_hdrBuffer, texCoords, u_chromaticAberrationStrength);
    vec3 bloomColor = texture(u_bloomBuffer, texCoords).rgb;
    hdrColor = hdrColor + bloomColor * u_bloomStrength; // Add bloom

    // Apply exposure
    //hdrColor = vec3(1.0) - exp(-hdrColor * u_exposure);

    // Choose tone mapping operator
    //hdrColor = reinhard(hdrColor);
    vec3 ldrColor = aces_approx(hdrColor);
    //hdrColor = uncharted2(hdrColor);

    // Gamma correction
    //float gamma = 2.2;
    //hdrColor = pow(hdrColor, vec3(1.0 / 2.2));

    //hdrColor *= u_brightness;

    ldrColor = (ldrColor - 0.5) * u_contrast + 0.5;

    vec3 grayscale = vec3(dot(ldrColor, vec3(0.2126, 0.7152, 0.0722)));
    ldrColor = mix(grayscale, ldrColor, u_saturation);

    float vig = vignette(texCoords, u_vignetteStrength, u_vignettePower);
    ldrColor *= vig;

    fragColor = vec4(ldrColor, 1.0);

}