#version 450

layout(std140, binding=0, set=0) buffer Data
{
	vec4 pos[];
};

layout(location = 0) out vec2 fragpos;

void main() {
    gl_Position = pos[gl_VertexIndex];
	fragpos = pos[gl_VertexIndex].xy;
}
