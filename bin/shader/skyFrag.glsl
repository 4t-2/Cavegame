#version 330 core

out vec4 color;

uniform float angle;

in vec4 pos;

void main()
{
	float y = (pos.y ) - angle + .3;

	y = min(y, 1);
	y = max(y, 0);

	y = smoothstep(0, 1, y);

	vec4 col1 = vec4(123. / 255., 169. / 255., 1, 1);
	vec4 col2 = vec4(181. / 255., 209. / 255., 1, 1);
	
    color = mix(col2, col1, y);
}
