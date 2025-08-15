#version 440 core

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoords;

uniform sampler2D uTexture;
uniform bool uUseTexture;

void main()
{
    if (uUseTexture) {
        float alpha = texture(uTexture, vTexCoords).r;
        FragColor = vec4(vColor.rgb, vColor.a * alpha);
    } else {
        FragColor = vColor;
    }
}