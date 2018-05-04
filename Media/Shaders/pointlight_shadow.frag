
#version 330

#include "pbr_common.head"

uniform sampler2D shadowMap;

uniform vec3 eyePos;
uniform vec3 lightPos;
uniform vec2 lightClipPlanes;
uniform vec2 shadowTexelSize;

in vec2 tex;
in vec3 wpos;
in vec3 wnorm;

in vec4 lvpos;	// for z calculation
in vec4 lcpos;	// for projective texturing

out vec4 my_FragColor0;

float ShadowPCF5x5(vec2 center, float d)
{
	float z;
	float shadow = 0.0;

	for (int i = -2; i < 3; ++i) {
		for (int j = -2; j < 3; ++j) {
			z = texture(shadowMap, center + vec2(i, j) * shadowTexelSize).r;
			shadow += ((z >= d) ? 1.0 : 0.0);
		}
	}

	return shadow * 0.04;
}

float ShadowVariance(vec2 center, float d)
{
	vec2 moments = texture(shadowMap, center).xy;

	float mean		= moments.x;
	float variance	= max(moments.y - moments.x * moments.x, 1e-5);
	float md		= mean - d;
	float chebychev	= variance / (variance + md * md);

	chebychev = smoothstep(0.1, 1.0, chebychev);

	// NEVER forget that d > mean in Chebychev's inequality!!!
	return max(((d <= mean) ? 1.0 : 0.0), chebychev);
}

void main()
{
	vec3 ldir = lightPos - wpos;
	float dist2 = dot(ldir, ldir);

	vec3 n = normalize(wnorm);
	vec3 v = normalize(eyePos - wpos);
	vec3 l = normalize(ldir);

	// calculate outgoing luminance
	vec3 luminance = CalculateLuminance_Point(n, v, l, dist2);

	// calculate shadow term
	vec2 projpos = (lcpos.xy / lcpos.w) * 0.5 + 0.5;

	float d = (-lvpos.z - lightClipPlanes.x) / (lightClipPlanes.y - lightClipPlanes.x);

	// unfiltered shadow
	//float z = texture(shadowMap, projpos).r;
	//float shadow = ((z >= d) ? 1.0 : 0.0);

	// PCF 5x5
	//float shadow = ShadowPCF5x5(projpos, d);

	// variance
	float shadow = ShadowVariance(projpos, d);

	my_FragColor0 = vec4(luminance * shadow, 1.0);
}
