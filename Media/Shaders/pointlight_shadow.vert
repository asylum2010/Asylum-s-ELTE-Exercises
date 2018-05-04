
#version 330

layout(location = 0) in vec3 my_Position;
layout(location = 1) in vec2 my_TexCoord0;
layout(location = 2) in vec3 my_Normal;

uniform mat4 matWorld;
uniform mat4 matWorldInv;
uniform mat4 matViewProj;

uniform mat4 matLightView;
uniform mat4 matLightProj;

out vec2 tex;
out vec3 wpos;
out vec3 wnorm;

out vec4 lvpos;	// for z calculation
out vec4 lcpos;	// for projective texturing

void main()
{
	vec4 pos = matWorld * vec4(my_Position, 1.0);

	wpos = pos.xyz;
	wnorm = (vec4(my_Normal, 0.0) * matWorldInv).xyz;
	tex = my_TexCoord0;

	lvpos = matLightView * pos;
	lcpos = matLightProj * lvpos;

	gl_Position = matViewProj * pos;
}
