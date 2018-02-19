
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
	// TODO:

	my_FragColor0 = baseColor;
}
