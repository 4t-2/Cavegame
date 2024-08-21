#version 330 core

in vec2 UVcoord;
in vec3 light;

out vec4 color;

uniform sampler2D textureSampler;

void main()
{
    color = texture(textureSampler, UVcoord) * vec4(light, 1);
	return;
	// Atmo haze                                                                      
	float distance_ = gl_FragCoord.z*0.001;                                          
	for(float threshold = 0.99600; threshold <= 0.99990; threshold += 0.00001)
	{
		if(gl_FragCoord.z >= threshold) {                                            
	       distance_ *= 1.0175;                                                     
	   }                                                                            
	}                                                                                

	// final color output                                                            
	color = mix(color, vec4(181./255., 209./255., 255./255, 1), distance_);      
}
