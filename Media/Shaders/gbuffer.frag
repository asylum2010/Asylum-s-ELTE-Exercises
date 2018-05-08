
#version 330

in vec2 tex;
in vec3 wnorm;

uniform float roughness;

out vec4 my_FragColor0;
out float my_FragColor1;

void main()
{
	vec3 n = normalize(wnorm);

	my_FragColor0 = vec4(n * 0.5 + vec3(0.5), roughness);
	my_FragColor1 = gl_FragCoord.z * 2.0 - 1.0;
}
