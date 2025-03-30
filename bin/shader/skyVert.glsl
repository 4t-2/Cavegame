#version 450

layout(set=0,binding=0) readonly buffer Vertex
{
	vec4 position[];
};

layout(set=1,binding=0) uniform Transform
{
	mat4 transform;
};

layout(location=0) out vec4 pos;

void main()
{
	pos = transform * position[gl_VertexIndex];
    gl_Position = pos;
}
