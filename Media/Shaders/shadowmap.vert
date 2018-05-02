
#version 330

layout(location = 0) in vec3 my_Position;

uniform mat4 matWorld;
uniform mat4 matView;
uniform mat4 matProj;

out vec4 vpos;

void main()
{
	vpos = matView * (matWorld * vec4(my_Position, 1.0));
	gl_Position = matProj * vpos;
}
