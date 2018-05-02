
#version 330

uniform vec2 clipPlanes;

out vec4 my_FragColor0;

in vec4 vpos;

void main()
{
	// NOTE: also good for variance shadow

	float linearDepth = (-vpos.z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);
	my_FragColor0 = vec4(linearDepth, linearDepth * linearDepth, 0.0, 0.0);
}
