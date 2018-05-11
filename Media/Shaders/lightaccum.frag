
#version 330

#include "pbr_common.head"

uniform mat4 matViewProjInv;
uniform vec3 eyePos;
uniform vec3 lightPos;
uniform vec3 lightColor;

uniform sampler2D gBufferNormals;
uniform sampler2D gBufferDepth;
uniform sampler2D gBufferBRDF;

out vec4 my_FragColor0;
out vec4 my_FragColor1;

in vec2 tex;

void main()
{
	ivec2 loc = ivec2(gl_FragCoord.xy);

	vec3 wnorm = texelFetch(gBufferNormals, loc, 0).xyz;
	float depth = texelFetch(gBufferDepth, loc, 0).x;
	vec4 params = texelFetch(gBufferBRDF, loc, 0);

	wnorm = wnorm * 2.0 - vec3(1.0);
	depth = depth * 2.0 - 1.0;

	vec4 spos = vec4(tex * 2.0 - vec2(1.0), depth, 1.0);
	vec4 wpos = matViewProjInv * spos;

	wpos /= wpos.w;

	vec3 ldir = lightPos - wpos.xyz;
	float dist2 = dot(ldir, ldir);

	vec3 n = wnorm;
	vec3 v = normalize(eyePos - wpos.xyz);
	vec3 l = normalize(ldir);
	vec3 h = normalize(v + l);

	float ndotv = max(dot(n, v), 0.0);
	float ndotl = max(dot(n, l), 0.0);
	float ndoth = max(dot(n, h), 0.0);
	float ldoth = max(dot(l, h), 0.0);

	// calculate BRDF
	float D = D_GGX(ndoth, params.w);
	vec3 F = F_Schlick(params.xyz, ldoth);
	float G = G_Smith_Schlick(ndotl, ndotv, params.w);

	// calculate illuminance
	float illum_diff = ndotl * ((ONE_OVER_PI * luminousFlux) / (QUAD_PI * dist2));

	vec3 cooktorrance = (D * F * G) / (4.0 * ndotv * ndotl + EPSILON);
	vec3 illum_spec = ndotl * ((cooktorrance * luminousFlux) / (QUAD_PI * dist2));

	my_FragColor0 = vec4(illum_diff * lightColor, 1.0);
	my_FragColor1 = vec4(illum_spec * lightColor, 1.0);

	// ignore sky
	if (depth > 1.0 - 1e-6) {
		my_FragColor0 = my_FragColor1 = vec4(0.0);
	}
}
