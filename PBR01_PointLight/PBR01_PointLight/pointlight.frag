
#version 330

#define ONE_OVER_PI	0.3183098861837906
#define PI			3.1415926535897932
#define QUAD_PI		12.566370614359172

uniform vec4 baseColor;
uniform vec3 lightPos;
uniform vec3 eyePos;
uniform float luminousFlux;

in vec2 tex;
in vec3 wpos;
in vec3 wnorm;

out vec4 my_FragColor0;

void main()
{
	vec3 ldir = lightPos - wpos;

	vec3 n = normalize(wnorm);
	//vec3 v = normalize(eyePos - wpos);
	vec3 l = normalize(ldir);

	// calculate the easy things
	float cos_theta = max(dot(n, l), 0.0);
	float dist2 = dot(ldir, ldir);

	// diffuse BRDF
	vec4 f_lambert = baseColor * ONE_OVER_PI;

	// calculate outgoing luminance
	vec3 luminance = cos_theta * ((f_lambert.xyz * luminousFlux) / (QUAD_PI * dist2));

	my_FragColor0 = vec4(luminance, 1.0);
}
