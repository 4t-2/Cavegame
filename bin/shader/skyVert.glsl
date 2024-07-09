#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 transform;
uniform mat4 mvp;

out vec4 pos;

void main()
{
	pos = mvp * transform * vec4(position, 1);
    gl_Position = pos;
}
