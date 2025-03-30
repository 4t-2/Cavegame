rversion 330

layout(location=0) in vec2 UVcoord;
layout(location=1) in vec4 fragColor;

layout(location=0) out vec4 color;

layout(set = 2, binding = 0) uniform sampler2D textureSampler;

void main()
{
    color = texture(textureSampler, UVcoord) * fragColor;
}
