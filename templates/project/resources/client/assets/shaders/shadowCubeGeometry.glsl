#version 460

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

in VS_OUT {
    vec4 fragPos;
} gs_in[];

out VS_OUT {
    vec4 fragPos;
} gs_out;


uniform mat4 u_shadowMatrices[6];

void main() {
    for (int face = 0; face < 6; ++face) {
        gl_Layer = face; // specify the layer to which we render
        for (int i = 0; i < 3; ++i) {
            gl_Position = u_shadowMatrices[face] * gs_in[i].fragPos;
            gs_out.fragPos = gs_in[i].fragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}