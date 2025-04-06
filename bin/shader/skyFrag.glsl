#version 450

layout(location=0) out vec4 color;

layout(set=2, binding=0) uniform Data
{
	float time;
	float rotx;
	float roty;
};

layout(location=0) in vec4 pos;

const float PI = 3.14159265359;

void main()
{
	float y = (pos.y);

	vec4 col1 = vec4(123. / 255., 169. / 255., 1, 1);
	vec4 col2 = vec4(181. / 255., 209. / 255., 1, 1);
	
    color = mix(col1, col2, y);
}
