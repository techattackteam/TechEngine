#version 330 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D depthTexture;

void main(){
    float depthValue = texture(depthTexture, texCoord).r;
    fragColor = vec4(vec3(depthValue), 1.0f);
}