#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 vertexUV;

uniform mat4 transform;
uniform mat4 mvp;
uniform vec3 shapeColor;
uniform mat4 textureTransform;
uniform vec3 norm;

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

	fragColor = vec4(shapeColor * l, 1);

    gl_Position = mvp * transform * vec4(position, 1);
}
