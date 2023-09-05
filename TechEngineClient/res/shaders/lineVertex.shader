#version 330 core

uniform vec3 start;
uniform vec3 end;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vec3 position = gl_VertexID == 0 ? start : end;
    gl_Position =  projection * view * vec4(position, 1.0f);
}

