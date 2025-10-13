#version 460 core

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D u_hdrBuffer;

uniform float u_exposure = 1.0;
#define EPSILON 0.0001
vec3 reinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 aces_approx(vec3 v) {
    v *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
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

uniform float u_saturation = 1.2;
uniform float u_contrast = 1.1;
uniform float u_brightness = 1.0;

void main() {
    vec3 hdrColor = texture(u_hdrBuffer, texCoords).rgb;

    // Apply exposure
    //hdrColor = vec3(1.0) - exp(-hdrColor * u_exposure);

    // Choose tone mapping operator
    hdrColor = reinhard(hdrColor);
    //hdrColor = aces_approx(hdrColor);
    //hdrColor = uncharted2(hdrColor);

    // Gamma correction
    float gamma = 2.2;
    hdrColor = pow(hdrColor, vec3(1.0 / 2.2));

    hdrColor *= u_brightness;

    // Apply Contrast
    hdrColor = (hdrColor - 0.5) * u_contrast + 0.5;

    // Apply Saturation
    vec3 grayscale = vec3(dot(hdrColor, vec3(0.2126, 0.7152, 0.0722)));
    hdrColor = mix(grayscale, hdrColor, u_saturation);

    fragColor = vec4(hdrColor, 1.0);

}