
#version 330

in vec2 tex;
in vec3 wnorm;

uniform float roughness;

out vec4 my_FragColor0;
out float my_FragColor1;
out vec4 my_FragColor2;

void main()
{
	vec3 n = normalize(wnorm);

	my_FragColor0 = vec4(n * 0.5 + vec3(0.5), 1.0);
	my_FragColor1 = gl_FragCoord.z;
	my_FragColor2 = vec4(vec3(0.04), roughness);
}
