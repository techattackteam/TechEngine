#version 460 core

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D u_hdrBuffer;
uniform sampler2D u_bloomBuffer;
uniform sampler3D u_colorGradingLUT;

// Bloom
uniform bool u_bloomEnabled = true;
uniform float u_bloomStrength = 1.0;

// Chromatic Aberration
uniform bool u_chromaticAberrationEnabled = false;
uniform float u_chromaticAberrationStrength = 1.0;
uniform float u_chromaticAberrationOffset = 1.0;

// Vignette
uniform bool u_vignetteEnabled = true;
uniform float u_vignetteStrength = 1.0;
uniform float u_vignettePower = 1.5;

// Lift-Gamma-Gain Color Wheels
uniform vec3 u_lift = vec3(0.0);
uniform float u_liftIntensity = 1.0;
uniform float u_gamma = 2.2;
uniform vec3 u_gammaRGB = vec3(1.0);
uniform float u_gammaIntensity = 1.0;
uniform vec3 u_gain = vec3(1.0);
uniform float u_gainIntensity = 1.0;

// Color Grading
uniform float u_exposure = 1.0;
uniform float u_saturation = 1.2;
uniform float u_contrast = 1.1;
uniform float u_brightness = 1.0;
uniform bool u_useLUT = false;
uniform float u_lutStrength = 1.0;
uniform int u_lutSize = 32;

// Film Grain
uniform bool u_filmGrainEnabled = false;
uniform float u_filmGrainIntensity = 0.05;
uniform float u_filmGrainSize = 1.0;
uniform float u_time = 0.0;


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

vec3 applyLiftGammaGain(vec3 color, vec3 lift, vec3 gamma, vec3 gain) {
    // A zero lift is neutral. The previous formula treated zero as -1 and
    // crushed darker color channels, washing textured materials toward gray.
    vec3 liftAdjusted = color + lift * (1.0 - color) * u_liftIntensity;
    vec3 gammaAdjusted = pow(max(liftAdjusted, vec3(0.0)), 1.0 / (gamma * u_gammaIntensity));
    vec3 gainAdjusted = gammaAdjusted * (gain * u_gainIntensity);
    return gainAdjusted;
}

vec3 applyLUT(vec3 color, sampler3D lut, int lutSize, float strength) {
    float scale = float(lutSize - 1) / float(lutSize);
    float offset = 0.5 / float(lutSize);

    vec3 uvw = color * scale + offset;

    vec3 lutColor = texture(lut, uvw).rgb;

    return mix(color, lutColor, strength);
}

vec3 adjustSaturation(vec3 color, float saturation) {
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    return mix(vec3(luminance), color, saturation);
}

vec3 adjustContrast(vec3 color, float contrast) {
    return (color - 0.5) * contrast + 0.5;
}

vec3 adjustBrightness(vec3 color, float brightness) {
    return color * brightness;
}

vec3 chromaticAberration(sampler2D hdrTexture, vec2 uv, float strength) {
    vec2 offset = (uv - 0.5) * 2.0; // -1 to 1
    offset = normalize(offset) * u_chromaticAberrationOffset;
    float dist = length(offset);

    vec2 direction = normalize(offset);
    float aberration = dist * strength * 0.01;

    float r = texture(hdrTexture, uv + direction * aberration).r;
    float g = texture(hdrTexture, uv).g;
    float b = texture(hdrTexture, uv - direction * aberration).b;

    return vec3(r, g, b);
}

float vignette(vec2 uv, float strength, float power) {
    vec2 offset = uv - 0.5;
    float dist = length(offset) * 2.0; // 0 at center, ~1.4 at corners

    float vig = 1.0 - smoothstep(0.0, 1.0, dist);
    vig = pow(vig, power);

    return mix(1.0, vig, strength);
}

float filmGrain(vec2 uv, float time, float intensity, float size) {
    float grain = fract(sin(dot(uv * size + time, vec2(12.9898, 78.233))) * 43758.5453);
    return (grain - 0.5) * intensity;
}

void main() {
    // Sample HDR color with optional chromatic aberration
    vec3 hdrColor = texture(u_hdrBuffer, texCoords).rgb;
    if (u_chromaticAberrationEnabled) {
        //hdrColor = chromaticAberration(u_hdrBuffer, texCoords, u_chromaticAberrationStrength);
    }

    // Bloom
    if (u_bloomEnabled) {
        //vec3 bloomColor = texture(u_bloomBuffer, texCoords).rgb;
        //hdrColor += bloomColor * u_bloomStrength;
    }

    // Exposure
    hdrColor = vec3(1.0) - exp(-hdrColor * u_exposure);

    // Choose tonemapping operator
    //hdrColor = reinhard(hdrColor);
    vec3 tonemapped = aces_approx(hdrColor);
    //hdrColor = uncharted2(hdrColor);

    // Lift-Gamma-Gain
    tonemapped = applyLiftGammaGain(tonemapped, u_lift, u_gammaRGB, u_gain);

    // Color correction
    tonemapped = adjustBrightness(tonemapped, u_brightness);
    tonemapped = adjustContrast(tonemapped, u_contrast);
    tonemapped = adjustSaturation(tonemapped, u_saturation);

    if (u_useLUT) {
        tonemapped = applyLUT(tonemapped, u_colorGradingLUT, u_lutSize, u_lutStrength);
    }

    if (u_vignetteEnabled) {
        float vig = vignette(texCoords, u_vignetteStrength, u_vignettePower);
        tonemapped *= vig;
    }

    if (u_filmGrainEnabled) {
        float grain = filmGrain(texCoords, u_time, u_filmGrainIntensity, u_filmGrainSize);
        tonemapped += grain;
    }

    tonemapped = clamp(tonemapped, 0.0, 1.0);
    fragColor = vec4(tonemapped, 1.0);
}