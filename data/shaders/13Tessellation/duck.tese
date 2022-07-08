#version 460 core

layout(triangles, equal_spacing, ccw) in;

in gl_PerVertex
{
	vec4 gl_Position;
} gl_in[];

out gl_PerVertex
{
	vec4 gl_Position;
};

layout (location = 0) in vec2 uvTescOut[];

layout (location = 0) out vec2 uvTeseOut;

vec2 interpolate2(in vec2 v0, in vec2 v1, in vec2 v2)
{
	return v0 * gl_TessCoord.x + v1 * gl_TessCoord.y + v2 * gl_TessCoord.z;
}

vec4 interpolate4(in vec4 v0, in vec4 v1, in vec4 v2)
{
	return v0 * gl_TessCoord.x + v1 * gl_TessCoord.y + v2 * gl_TessCoord.z;
}

void main()
{
	gl_Position = interpolate4(gl_in[0].gl_Position,gl_in[1].gl_Position, gl_in[2].gl_Position);
	uvTeseOut = interpolate2(uvTescOut[0], uvTescOut[1], uvTescOut[2]);
};
