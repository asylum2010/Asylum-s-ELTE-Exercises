
#version 330

layout(location = 0) in vec3 my_Position;

uniform mat4 matWorld;

void main()
{
	gl_Position = matWorld * vec4(my_Position, 1.0);
}
