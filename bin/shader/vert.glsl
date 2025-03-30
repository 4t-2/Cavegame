#version 450

layout(set=0,binding=0) readonly buffer Vertex
{
	vec3 position;
	vec2 vertexUV;
};

layout(set=1, binding=0) uniform Data
{
	mat4 transform;
	mat4 mvp;
	vec3 shapeColor;
	mat4 textureTransform;
	vec3 norm;
	
	float x0y0;
	float x1y0;
	float x0y1;
	float x1y1;
};

layout(location=0) out vec2 UVcoord;
layout(location=1) out vec4 fragColor;

void main()
{
    UVcoord = vec2((textureTransform * vec4(vertexUV, 0, 1)).xy);

	float l = length(norm * vec3(151./255., 84./85., 202./255.));

	if(norm.y < 0)
	{
		l = 25./51.;
	}

	float ao = 1;
	ao -= (1 - position.x) * (1 - position.y) * x0y0;
	ao -= position.x * position.y * x1y1;
	ao -= position.x * (1 - position.y) * x1y0;
	ao -= (1 - position.x) * position.y * x0y1;

	fragColor = vec4(shapeColor * l * ao, 1);

    gl_Position = mvp * transform * vec4(position, 1);
}
