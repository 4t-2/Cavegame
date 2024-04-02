#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 vertexUV;

uniform mat4 transform;
uniform mat4 mvp;
uniform vec3 shapeColor;
uniform mat4 textureTransform;
uniform vec3 norm;

uniform float x0y0;
uniform float x1y0;
uniform float x0y1;
uniform float x1y1;

out vec2 UVcoord;
out vec4 fragColor;

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
