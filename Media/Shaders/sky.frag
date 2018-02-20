
#version 330

uniform samplerCube skyCube;

in vec3 vdir;

out vec4 my_FragColor0;

void main()
{
	vec4 base = texture(skyCube, vdir);
	my_FragColor0 = vec4(base.rgb, 1.0);
}
