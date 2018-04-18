
#version 330

layout(location = 0) in vec3 my_Position;

uniform mat4 matWorld;
uniform mat4 matViewProj;

void main()
{
	gl_Position = matViewProj * (matWorld * vec4(my_Position, 1.0));
}
