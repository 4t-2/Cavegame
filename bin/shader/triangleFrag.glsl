#version 450

layout(location=0) in vec2 pos;
layout(location=0) out vec4 outColor;

void main() {
    outColor = vec4(pos,1,1);
}
