#version 460 core

#include <data/shaders/13Tessellation/GLDecl.glsl>



layout (vertices = 3) out;

layout (location = 0) in vec2 uv_in[];
layout (location = 1) in vec3 worldPos_in[];

in gl_PerVertex {
	vec4 gl_Position;
} gl_in[];

out gl_PerVertex {
	vec4 gl_Position;
} gl_out[];

layout (location = 0) out vec2 uvTescOut[];

float getTessLevel(float distance0, float distance1)
{
	const float distanceScale1 = 7.0;
	const float distanceScale2 = 10.0;
	const float avgDistance = (distance0 + distance1) / 2.0;

	if (avgDistance <= distanceScale1 * tessellationScale)
	{
		return 5.0;
	}
	else if (avgDistance <= distanceScale2 * tessellationScale)
	{
		return 3.0;
	}

	return 1.0;
}

void main()
{	
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	uvTescOut[gl_InvocationID] = uv_in[gl_InvocationID];

	vec3 c = cameraPos.xyz;

	float eyeToVertexDistance0 = distance(c, worldPos_in[0]);
	float eyeToVertexDistance1 = distance(c, worldPos_in[1]);
	float eyeToVertexDistance2 = distance(c, worldPos_in[2]);

	// Calculate the tessellation levels
	gl_TessLevelOuter[0] = getTessLevel(eyeToVertexDistance1, eyeToVertexDistance2);
	gl_TessLevelOuter[1] = getTessLevel(eyeToVertexDistance2, eyeToVertexDistance0);
	gl_TessLevelOuter[2] = getTessLevel(eyeToVertexDistance0, eyeToVertexDistance1);
	gl_TessLevelInner[0] = gl_TessLevelOuter[2];
};
