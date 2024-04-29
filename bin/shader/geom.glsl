#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 17) out;

out vec4 fragColor;

uniform mat4 mvp;
 
void createVertex(vec4 pos, vec4 offset)
{
	gl_Position = mvp * (pos + offset);
	fragColor = vec4(offset.xyz, 1);
	EmitVertex();
}

void main() {    
	createVertex(gl_in[0].gl_Position, vec4(0, 0, 0, 0));

	createVertex(gl_in[0].gl_Position, vec4(1, 0, 0, 0));
	createVertex(gl_in[0].gl_Position, vec4(0, 1, 0, 0));
	createVertex(gl_in[0].gl_Position, vec4(1, 1, 0, 0)); // front
	
	createVertex(gl_in[0].gl_Position, vec4(0, 1, 1, 0));
	createVertex(gl_in[0].gl_Position, vec4(1, 1, 1, 0)); // top
	
	createVertex(gl_in[0].gl_Position, vec4(0, 0, 1, 0));
	createVertex(gl_in[0].gl_Position, vec4(1, 0, 1, 0)); // back
	
	createVertex(gl_in[0].gl_Position, vec4(1, 1, 1, 0));
	createVertex(gl_in[0].gl_Position, vec4(1, 1, 1, 0));
	
	createVertex(gl_in[0].gl_Position, vec4(1, 1, 0, 0));
	createVertex(gl_in[0].gl_Position, vec4(1, 0, 1, 0));
	createVertex(gl_in[0].gl_Position, vec4(1, 0, 0, 0)); // left
	
	createVertex(gl_in[0].gl_Position, vec4(0, 0, 1, 0));
	createVertex(gl_in[0].gl_Position, vec4(0, 0, 0, 0)); // bottom

	createVertex(gl_in[0].gl_Position, vec4(0, 1, 1, 0));
	createVertex(gl_in[0].gl_Position, vec4(0, 1, 0, 0)); // right
	
	// right

    EndPrimitive();
} 
