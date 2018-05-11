
#version 330

uniform vec4 baseColor;

uniform sampler2D accumDiffuse;
uniform sampler2D accumSpecular;

out vec4 my_FragColor0;

//layout(early_fragment_tests) in;	// NOTE: GL 4.3
void main()
{
	ivec2 loc = ivec2(gl_FragCoord.xy);

	vec3 illum_diff = texelFetch(accumDiffuse, loc, 0).rgb;
	vec3 illum_spec = texelFetch(accumSpecular, loc, 0).rgb;

	my_FragColor0 = vec4(baseColor.rgb * illum_diff + illum_spec, 1.0);
}
