
#version 330

out vec4 my_FragColor0;

uniform sampler2D sampler0;

in vec2 tex;

const float A = 0.22;
const float B = 0.30;
const float C = 0.10;
const float D = 0.20;
const float E = 0.01;
const float F = 0.30;
const float W = 11.2;

vec3 Uncharted2Tonemap(vec3 x)
{
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

void main()
{
	// usually you calculate this by measuring the average luminance
	const float exposure = 1.0;

	vec4 base = texture(sampler0, tex);
	vec3 lincolor = Uncharted2Tonemap(base.rgb * exposure);
	vec3 invlinwhite = 1.0 / Uncharted2Tonemap(vec3(W));

	base.rgb = lincolor * invlinwhite;

	my_FragColor0 = base;
}
