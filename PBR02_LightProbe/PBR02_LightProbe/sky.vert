
#version 330

layout(location = 0) in vec3 my_Position;

uniform mat4 matWorld;
uniform mat4 matViewProj;

out vec3 wpos;

void main()
{
	vec4 pos = matWorld * vec4(my_Position, 1.0);
	wpos = pos.xyz;

	gl_Position = matViewProj * pos;
}
