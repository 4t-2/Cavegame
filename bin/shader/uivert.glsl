#version 450

layout(set=0,binding=0) readonly buffer Vertex
{
	vec3 position;
	vec2 vertexUV;
};

layout(set=1,binding=0) readonly buffer Data
{
	mat4 transform;
	mat4 mvp;
	vec4 shapeColor;
	mat4 textureTransform;
};

layout(location=0) out vec2 UVcoord;
layout(location=1) out vec4 fragColor;

void main()
{
    UVcoord = vec2((textureTransform * vec4(vertexUV, 0, 1)).xy);

	fragColor = shapeColor;

    gl_Position = mvp * transform * vec4(position, 1);
}
