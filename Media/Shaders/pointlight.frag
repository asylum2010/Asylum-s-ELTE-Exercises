
#version 330

#define ONE_OVER_PI	0.3183098861837906
#define PI			3.1415926535897932
#define QUAD_PI		12.566370614359172
#define EPSILON		1e-5					// to avoid division with zero

uniform vec3 eyePos;

// material params
uniform vec4 baseColor;
uniform float roughness;

// light params
uniform vec3 lightPos;
uniform float luminousFlux;

in vec2 tex;
in vec3 wpos;
in vec3 wnorm;

out vec4 my_FragColor0;

vec3 F_Schlick(vec3 f0, float u)
{
	return f0 + (vec3(1.0) - f0) * pow(1.0 - u + EPSILON, 5.0);
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
	vec3 ldir = lightPos - wpos;

	vec3 n = normalize(wnorm);
	vec3 v = normalize(eyePos - wpos);
	vec3 l = normalize(ldir);
	vec3 h = normalize(v + l);

	// calculate the easy things
	float ndotv = max(dot(n, v), 0.0);
	float ndotl = max(dot(n, l), 0.0);	// cos(theta)
	float ndoth = max(dot(n, h), 0.0);
	float ldoth = max(dot(l, h), 0.0);

	float dist2 = dot(ldir, ldir);

	// BRDF diffuse term (Lambert)
	vec4 f_lambert = baseColor * ONE_OVER_PI;

	// BRDF specular term (Cook-Torrance)
	float D = D_GGX(ndoth);
	vec3 F = F_Schlick(vec3(0.04), ldoth);
	float G = G_Smith_Schlick(ndotl, ndotv);

	vec3 f_cooktorrance = (D * F * G) / (4.0 * ndotv * ndotl + EPSILON);

	// calculate outgoing luminance
	vec3 brdf = f_lambert.xyz + f_cooktorrance;
	vec3 luminance = ndotl * ((brdf * luminousFlux) / (QUAD_PI * dist2));

	// won't work for transparent materials (need the BSDF model)
	my_FragColor0 = vec4(luminance, 1.0);
}
