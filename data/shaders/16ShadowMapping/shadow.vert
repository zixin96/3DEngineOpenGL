//
#version 460 core

#include <data/shaders/16ShadowMapping/GLBufferDeclarations.glsl>

layout(std140, binding = 0) uniform PerFrameData
{
	mat4 view;
	mat4 proj;
};

vec3 getPosition(int i)
{
	return vec3(in_Vertices[i].p[0], in_Vertices[i].p[1], in_Vertices[i].p[2]);
}

void main()
{
	// since we use glDrawElements, gl_BaseInstance is 0, which is fine 
	// b/c we only have a single model matrix that applies to all meshes in the scene
	mat4 MVP = proj * view * in_ModelMatrices[gl_BaseInstance]; 

	gl_Position = MVP * vec4(getPosition(gl_VertexID), 1.0);
}
