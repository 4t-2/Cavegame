#version 330 core

in vec2 UVcoord;
in vec4 fragColor;

uniform float time;
uniform float rotx;
uniform float roty;

out vec4 color;

uniform sampler2D textureSampler;

void main()
{
    vec4 baseColor = texture(textureSampler, UVcoord) * fragColor;

	// Atmo haze
    float distance_ = gl_FragCoord.z*0.001;
    for(float threshold = 0.99600; threshold <= 0.99990; threshold += 0.00001) {
        if(gl_FragCoord.z >= threshold) {
            distance_ *= 1.0175;
        }
    }

    // final color output
    color = mix(baseColor, vec4(181./255., 209./255., 255./255, 1), distance_);
}
