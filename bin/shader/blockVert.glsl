#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 lighting;

uniform mat4 mvp;

out vec2 UVcoord;
out vec3 light;

void main()
{
    UVcoord = vertexUV;

	light = lighting;

    gl_Position = mvp * vec4(position, 1);
}
