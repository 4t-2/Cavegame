#version 330 core
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 36) out;

out vec4 fragColor;

uniform mat4 mvp;

mat4 transform = mat4(
gl_in[0].gl_Position.x, gl_in[1].gl_Position.y, gl_in[1].gl_Position.z, 0,
gl_in[0].gl_Position.w, gl_in[1].gl_Position.x, gl_in[1].gl_Position.y, 0,
gl_in[1].gl_Position.z, gl_in[1].gl_Position.w, gl_in[2].gl_Position.x, 0,
gl_in[2].gl_Position.y, gl_in[2].gl_Position.z, gl_in[2].gl_Position.w, 1);
void createVertex(vec4 offset, float aoc)
{
	gl_Position = mvp * vec4((transform * offset).xyz, 1);
	fragColor = vec4(vec3(1, 1, 1) * aoc, 1);
	EmitVertex();
}

int extract(int buf, int size, int start) {
	return (((1 << size) -1) & (buf >> start));
}

float bufToAoc(int buf)
{
	float f = 1 - (float(buf) * .2);
	return f;
}

void main() {
int a = 1 & 1;
	int i = floatBitsToInt(gl_in[3].gl_Position.x);
	float x0y0 = bufToAoc(extract(i, 2, 0));
	float x0y1 = bufToAoc(extract(i, 2, 2));
	float x1y0 = bufToAoc(extract(i, 2, 4));
	float x1y1 = bufToAoc(extract(i, 2, 6));

	createVertex(vec4(0, 1, 0, 1), x0y0); // top
	createVertex(vec4(1, 1, 0, 1), x1y0);
	createVertex(vec4(0, 1, 1, 1), x0y1);

	createVertex(vec4(1, 1, 0, 1), x1y0);
	createVertex(vec4(0, 1, 1, 1), x0y1);
	createVertex(vec4(1, 1, 1, 1), x1y1);

	EndPrimitive();

	createVertex(vec4(0, 0, 0, 1), 1); // bottom
	createVertex(vec4(1, 0, 0, 1), 1);
	createVertex(vec4(0, 0, 1, 1), 1);

	createVertex(vec4(1, 0, 0, 1), 1);
	createVertex(vec4(0, 0, 1, 1), 1);
	createVertex(vec4(1, 0, 1, 1), 1);

	EndPrimitive();

	createVertex(vec4(0, 0, 0, 1), 1); // south (-z)
	createVertex(vec4(1, 0, 0, 1), 1);
	createVertex(vec4(0, 1, 0, 1), 1);

	createVertex(vec4(1, 0, 0, 1), 1);
	createVertex(vec4(0, 1, 0, 1), 1);
	createVertex(vec4(1, 1, 0, 1), 1);

	EndPrimitive();

	createVertex(vec4(0, 0, 1, 1), 1); // north (+z)
	createVertex(vec4(1, 0, 1, 1), 1);
	createVertex(vec4(0, 1, 1, 1), 1);

	createVertex(vec4(1, 0, 1, 1), 1);
	createVertex(vec4(0, 1, 1, 1), 1);
	createVertex(vec4(1, 1, 1, 1), 1);

	EndPrimitive();

	createVertex(vec4(0, 0, 0, 1), 1); // west (-x)
	createVertex(vec4(0, 1, 0, 1), 1);
	createVertex(vec4(0, 0, 1, 1), 1);

	createVertex(vec4(0, 1, 0, 1), 1);
	createVertex(vec4(0, 0, 1, 1), 1);
	createVertex(vec4(0, 1, 1, 1), 1);

	EndPrimitive();

	createVertex(vec4(1, 0, 0, 1), 1); // east (+x)
	createVertex(vec4(1, 1, 0, 1), 1);
	createVertex(vec4(1, 0, 1, 1), 1);

	createVertex(vec4(1, 1, 0, 1), 1);
	createVertex(vec4(1, 0, 1, 1), 1);
	createVertex(vec4(1, 1, 1, 1), 1);

   	EndPrimitive();
}
/*
old

4 vec3 - 12
4 vec2 - 8
mat4 - 16
vec3 - 3
mat4 - 16
vec3 - 3
vec4 - 4

248 bytes

new

vec3 pos - 3 - 12
vec3 size - 3 - 12
vec3 rotation - 3 - 12
vec3 col - 3 // 3
6 f // 6 2 bits - 1.5 bytes
6 shorts - 12
6 floats - 24
6 bytes - 6

178

96

DECIDED

mat4 transform - 4

48

1 byte shade
.25 byte rotation
1.25 byte startuvx
1.25 byte startuvy
.5 byte enduvx
.5 byte enduvy
3  colour
*/
