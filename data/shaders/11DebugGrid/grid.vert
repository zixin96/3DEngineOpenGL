#version 460 core

// This vertex shader is used to generate and transform grid vertices

#include <data/shaders/11DebugGrid/GridParameters.glsl>
#include <data/shaders/11DebugGrid/GLBufferDecl.glsl>

layout (location=0) out vec2 uv;

void main()
{
	mat4 MVP = proj * view;

	int idx = indices[gl_VertexID];
	vec3 position = pos[idx] * gridSize;

	gl_Position = MVP * vec4(position, 1.0);
	uv = position.xz;
}
