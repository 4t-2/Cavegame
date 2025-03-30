#version 450

struct Vert
{
	vec4 position;
	vec4 vertexUV;
	vec4 lighting;
};

layout(set=0,binding=0) readonly buffer Vertex
{
	Vert vertex[];
};

layout(set=1,binding=0) uniform MVP
{
	mat4 mvp;
};

layout(location = 0) out vec2 UVcoord;
layout(location = 1) out vec3 light;

void main()
{
    UVcoord = vertex[gl_VertexIndex].vertexUV.xy;

	light = vertex[gl_VertexIndex].lighting.xyz;

    gl_Position = mvp * vec4(vertex[gl_VertexIndex].position.xyz, 1);
}
