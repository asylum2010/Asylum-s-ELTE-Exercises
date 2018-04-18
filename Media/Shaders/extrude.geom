
#version 330

uniform mat4 matViewProj;
uniform vec3 lightPos;

layout(triangles_adjacency) in;
layout(triangle_strip, max_vertices = 18) out;

void ExtrudeIfSilhouette(vec4 v1, vec4 v2, vec4 v3)
{
	// NOTE: (v1, v3) is the edge to be extruded
	vec4	extruded1;
	vec4	extruded2;
	vec3	a = v2.xyz - v1.xyz;
	vec3	b = v3.xyz - v1.xyz;
	vec4	planeeq;
	float	dist;

	// calculate plane equation of triangle
	planeeq.xyz = cross(a, b);
	planeeq.w = -dot(planeeq.xyz, v1.xyz);

	// calculate distance from light
	dist = dot(planeeq, vec4(lightPos, 1.0));

	if (dist < 0.0) {
		// faces away from light, extrude edge
		extruded1 = vec4(v1.xyz - lightPos, 0.0);
		extruded2 = vec4(v3.xyz - lightPos, 0.0);

		gl_Position = matViewProj * v3;
		EmitVertex();

		gl_Position = matViewProj * v1;
		EmitVertex();

		gl_Position = matViewProj * extruded2;
		EmitVertex();

		gl_Position = matViewProj * extruded1;
		EmitVertex();

		EndPrimitive();
	}
}

void main()
{
	// NOTE: base triangle is (0, 2, 4)
	vec4 v0 = gl_in[0].gl_Position;
	vec4 v1 = gl_in[1].gl_Position;
	vec4 v2 = gl_in[2].gl_Position;
	vec4 v3 = gl_in[3].gl_Position;
	vec4 v4 = gl_in[4].gl_Position;
	vec4 v5 = gl_in[5].gl_Position;

	// only interested in cases when the base triangle faces the light
	vec4 planeeq;
	vec3 a = v2.xyz - v0.xyz;
	vec3 b = v4.xyz - v0.xyz;
	float dist;

	planeeq.xyz = cross(a, b);
	planeeq.w = -dot(planeeq.xyz, v0.xyz);

	dist = dot(planeeq, vec4(lightPos, 1));

	if (dist > 0.0) {
		ExtrudeIfSilhouette(v0, v1, v2);
		ExtrudeIfSilhouette(v2, v3, v4);
		ExtrudeIfSilhouette(v4, v5, v0);

		// front cap
		gl_Position = matViewProj * v0;
		EmitVertex();

		gl_Position = matViewProj * v2;
		EmitVertex();

		gl_Position = matViewProj * v4;
		EmitVertex();

		EndPrimitive();

		// back cap
		gl_Position = matViewProj * vec4(v0.xyz - lightPos, 0.0);
		EmitVertex();

		gl_Position = matViewProj * vec4(v4.xyz - lightPos, 0.0);
		EmitVertex();

		gl_Position = matViewProj * vec4(v2.xyz - lightPos, 0.0);
		EmitVertex();

		EndPrimitive();
	}
}
