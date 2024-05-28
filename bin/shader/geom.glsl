#version 330 core
layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 36) out;

out vec4 fragColor;
out vec2 UVcoord;

uniform mat4 mvp;

uniform sampler2D elementDataSampler;

const vec3 tintArr[3] = vec3[3](
	vec3(1., 1., 1.), // white
	vec3(118. / 255., 194. / 255., 81. / 255.), // grass
	vec3(85. / 255., 178. / 255., 37. / 255.) // foliage
);

// element data
/*

per side

f uvx 1.5
f uvy 1.5
f six 1.5
f siy 1.5
2 bit rot .25
1 bit exist mask

52 bytes
64 bytes

per block

mat43 48 bytes

shade 1 byte
2 bits tint

id 2 byte

57.5 bytes
64 bytes

*/

mat4 transform = mat4(
gl_in[0].gl_Position.x, gl_in[1].gl_Position.y, gl_in[1].gl_Position.z, 0,
gl_in[0].gl_Position.w, gl_in[1].gl_Position.x, gl_in[1].gl_Position.y, 0,
gl_in[1].gl_Position.z, gl_in[1].gl_Position.w, gl_in[2].gl_Position.x, 0,
gl_in[2].gl_Position.y, gl_in[2].gl_Position.z, gl_in[2].gl_Position.w, 1);

int extract(int buf, int size, int start) {
	return (((1 << size) -1) & (buf >> start));
}

float bufToAoc(int buf)
{
	float f = 1 - (float(buf) * .2);
	return f;
}

vec4 idToSample(int id)
{
	float x = float(extract(id, 7, 0));
	float y = float(extract(id, 7, 7));

	x = x / 128.0;
	y = y / 128.0;

	return texture(elementDataSampler, vec2(x, y));
}

vec2 uv00 = vec2(0, 0);
vec2 uv01 = vec2(0, 0);
vec2 uv10 = vec2(0, 0);
vec2 uv11 = vec2(0, 0);
float x0y0 = 0;
float x0y1 = 0;
float x1y0 = 0;
float x1y1 = 0;
vec3 tint = vec3(0, 0);
int exists = 0;

void updateConsts(float f1, float f2, float f3)
{
	int i = floatBitsToInt(f1);
	int b1 = floatBitsToInt(f2);
	int b2 = floatBitsToInt(f3);

	exists = extract(b1, 1, 31);

	if(exists == 1)
	{
		x0y0 = bufToAoc(extract(i, 2, 0));
		x0y1 = bufToAoc(extract(i, 2, 2));
		x1y0 = bufToAoc(extract(i, 2, 4));
		x1y1 = bufToAoc(extract(i, 2, 6));
		
		float sx = extract(b1, 10, 0);
		float sy = extract(b1, 10, 16);
		float ex = extract(b2, 4, 0) + 1;
		float ey = extract(b2, 4, 16) + 1;
		vec2 sta = vec2(sx / 512.0, sy / 512.0);
		vec2 siz = vec2(ex / 512.0, ey / 512.0);
    
		vec2 uvStart = sta;
		vec2 uvEnd = sta + siz;
    
		uv00 = uvStart;
		uv01 = vec2(uvStart.x, uvEnd.y);
		uv10 = vec2(uvEnd.x, uvStart.y);
		uv11 = uvEnd;
    
		int tintid = extract(i, 2, 8);
		tint = tintArr[tintid];
	}
}

void createVertex(vec4 offset, float aoc, vec2 uv)
{
	gl_Position = mvp * vec4((transform * offset).xyz, 1);
	fragColor = vec4(tint * aoc, 1);
	UVcoord = uv;
	EmitVertex();
}

void main() {
	int id = floatBitsToInt(gl_in[4].gl_Position.z);

	vec4 buf1 = idToSample((id * 3));
	vec4 buf2 = idToSample((id * 3) + 1);
	vec4 buf3 = idToSample((id * 3) + 2);

	{
		updateConsts(gl_in[3].gl_Position.x, buf1.x, buf1.y);

		if(exists == 1)
		{
			createVertex(vec4(0, 1, 0, 1), x0y0, uv00); // top
			createVertex(vec4(1, 1, 0, 1), x1y0, uv10);
			createVertex(vec4(0, 1, 1, 1), x0y1, uv01);
	
			createVertex(vec4(1, 1, 0, 1), x1y0, uv10);
			createVertex(vec4(0, 1, 1, 1), x0y1, uv01);
			createVertex(vec4(1, 1, 1, 1), x1y1, uv11);
	
			EndPrimitive();
		}
	}

	{
		updateConsts(gl_in[3].gl_Position.y, buf1.z, buf1.w);

		if(exists == 1)
		{
			createVertex(vec4(0, 0, 0, 1), x0y1, uv01); // bottom
			createVertex(vec4(1, 0, 0, 1), x1y1, uv11);
			createVertex(vec4(0, 0, 1, 1), x0y0, uv00);
                                                 
			createVertex(vec4(1, 0, 0, 1), x1y1, uv11);
			createVertex(vec4(0, 0, 1, 1), x0y0, uv00);
			createVertex(vec4(1, 0, 1, 1), x1y0, uv10);
            
			EndPrimitive();
		}
	}

	{
		updateConsts(gl_in[3].gl_Position.z, buf2.x, buf2.y);

		if(exists == 1)
		{
			createVertex(vec4(0, 0, 0, 1), x1y1, uv11); // south (-z)
			createVertex(vec4(1, 0, 0, 1), x0y1, uv01);
			createVertex(vec4(0, 1, 0, 1), x1y0, uv10);
            
			createVertex(vec4(1, 0, 0, 1), x0y1, uv01);
			createVertex(vec4(0, 1, 0, 1), x1y0, uv10);
			createVertex(vec4(1, 1, 0, 1), x0y0, uv00);
            
			EndPrimitive();
		}
	}

	{
		updateConsts(gl_in[3].gl_Position.w, buf2.z, buf2.w);

		if(exists == 1)
		{
			createVertex(vec4(0, 0, 1, 1), x0y1, uv01); // north (+z)
			createVertex(vec4(1, 0, 1, 1), x1y1, uv11);
			createVertex(vec4(0, 1, 1, 1), x0y0, uv00);
            
			createVertex(vec4(1, 0, 1, 1), x1y1, uv11);
			createVertex(vec4(0, 1, 1, 1), x0y0, uv00);
			createVertex(vec4(1, 1, 1, 1), x1y0, uv10);
            
			EndPrimitive();
		}
	}

	{
		updateConsts(gl_in[4].gl_Position.x, buf3.x, buf3.y);

		if(exists == 1)
		{
			createVertex(vec4(0, 0, 0, 1), x0y1, uv01); // west (-x)
			createVertex(vec4(0, 1, 0, 1), x0y0, uv00);
			createVertex(vec4(0, 0, 1, 1), x1y1, uv11);
            
			createVertex(vec4(0, 1, 0, 1), x0y0, uv00);
			createVertex(vec4(0, 0, 1, 1), x1y1, uv11);
			createVertex(vec4(0, 1, 1, 1), x1y0, uv10);
            
			EndPrimitive();
		}
	}

	{
		updateConsts(gl_in[4].gl_Position.y, buf3.z, buf3.w);

		if(exists == 1)
		{
			createVertex(vec4(1, 0, 0, 1), x1y1, uv11); // east (+x)
			createVertex(vec4(1, 1, 0, 1), x1y0, uv10);
			createVertex(vec4(1, 0, 1, 1), x0y1, uv01);
            
			createVertex(vec4(1, 1, 0, 1), x1y0, uv10);
			createVertex(vec4(1, 0, 1, 1), x0y1, uv01);
			createVertex(vec4(1, 1, 1, 1), x0y0, uv00);
            
   			EndPrimitive();
		}
	}
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
3  colour

48

1 byte shade
.25 byte rotation
1.25 byte startuvx
1.25 byte startuvy
.5 byte enduvx
.5 byte enduvy
*/
