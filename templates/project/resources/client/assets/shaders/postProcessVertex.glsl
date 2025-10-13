#version 460

out vec2 texCoords;

// Vertices for a fullscreen quad
vec3 positions[6] = vec3[](
vec3(-1.0, 1.0, 0.0),
vec3(-1.0, -1.0, 0.0),
vec3(1.0, -1.0, 0.0),
vec3(-1.0, 1.0, 0.0),
vec3(1.0, -1.0, 0.0),
vec3(1.0, 1.0, 0.0)
);

// Texture coordinates for a fullscreen quad
vec2 texCoordsArray[6] = vec2[](
vec2(0.0, 1.0),
vec2(0.0, 0.0),
vec2(1.0, 0.0),
vec2(0.0, 1.0),
vec2(1.0, 0.0),
vec2(1.0, 1.0)
);


void main() {
    gl_Position = vec4(positions[gl_VertexID], 1.0);
    texCoords = texCoordsArray[gl_VertexID];
}