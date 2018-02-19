
#version 330

out vec2 tex;

void main()
{
	vec4 positions[4] = vec4[4](
		vec4(-1.0, -1.0, 0.0, 1.0),
		vec4(1.0, -1.0, 0.0, 1.0),
		vec4(-1.0, 1.0, 0.0, 1.0),
		vec4(1.0, 1.0, 0.0, 1.0));

	vec2 texcoords[4] = vec2[4](
		vec2(0.0, 0.0),
		vec2(1.0, 0.0),
		vec2(0.0, 1.0),
		vec2(1.0, 1.0));

	tex = texcoords[gl_VertexID];
	gl_Position = positions[gl_VertexID];
}
