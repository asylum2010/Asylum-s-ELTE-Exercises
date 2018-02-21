
#version 330

#define ONE_OVER_PI	0.3183098861837906
#define PI			3.1415926535897932
#define QUAD_PI		12.566370614359172
#define EPSILON		1e-5					// to avoid division with zero

uniform vec3 eyePos;

// material params
uniform vec4 baseColor;
uniform float roughness;
uniform float metalness;

// preintegrated irradiance and BRDF textures
uniform samplerCube irradianceDiffuse;
uniform samplerCube irradianceSpecular;
uniform sampler2D brdfLUT;

in vec2 tex;
in vec3 wpos;
in vec3 wnorm;

out vec4 my_FragColor0;

vec3 F_Schlick(vec3 f0, float u)
{
	return f0 + (vec3(1.0) - f0) * pow(1.0 - u, 5.0);
}

float D_GGX(float ndoth)
{
	// Disney's suggestion
	float a = roughness * roughness;
	float a2 = a * a;

	// optimized formula for the GPU
	float d = (ndoth * a2 - ndoth) * ndoth + 1.0;

	return a2 / (PI * d * d + EPSILON);
}

float G_Smith_Schlick(float ndotl, float ndotv)
{
	// Disney's suggestion
	float a = roughness + 1.0;
	float k = a * a * 0.125;

	// shadowing/masking functions
	float G1_l = ndotl * (1 - k) + k + EPSILON;
	float G1_v = ndotv * (1 - k) + k + EPSILON;

	// could be optimized out due to Cook-Torrance's denominator
	return (ndotl / G1_l) * (ndotv / G1_v);
}

void main()
{
	const int numMipLevels = 8;	// this is how it was generated

	vec3 n = normalize(wnorm);
	vec3 v = normalize(eyePos - wpos);
	vec3 r = 2.0 * dot(v, n) * n - v;

	float ndotv = max(dot(n, v), 0.0);
	float miplevel = roughness * float(numMipLevels - 1);

	// calculate luminance from preintegrated illuminances
	vec3 f_lambert			= baseColor.rgb * ONE_OVER_PI;
	vec3 diffuse_rad		= texture(irradianceDiffuse, n).rgb * f_lambert;
	vec3 specular_rad		= textureLod(irradianceSpecular, r, miplevel).rgb;
	vec2 f0_scale_bias		= texture(brdfLUT, vec2(ndotv, roughness)).rg;

	if (metalness > 0.99)
		diffuse_rad = vec3(0.0);

	// calculate Fresnel term
	vec3 F0 = mix(vec3(0.04), baseColor.rgb, metalness);
	vec3 F = F0 * f0_scale_bias.x + vec3(f0_scale_bias.y);

	// calculate output luminance
	my_FragColor0.rgb = diffuse_rad + specular_rad * F;
	my_FragColor0.a = 1.0;
}
