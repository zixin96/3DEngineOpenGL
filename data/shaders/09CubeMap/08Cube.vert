#version 460 core

#include <data/shaders/09CubeMap/GLBufferDecl.h>

layout (location=0) out vec3 dir;

const vec3 pos[8] = vec3[8](
	vec3(-1.0,-1.0, 1.0),
	vec3( 1.0,-1.0, 1.0),
	vec3( 1.0, 1.0, 1.0),
	vec3(-1.0, 1.0, 1.0),

	vec3(-1.0,-1.0,-1.0),
	vec3( 1.0,-1.0,-1.0),
	vec3( 1.0, 1.0,-1.0),
	vec3(-1.0, 1.0,-1.0)
);

const int indices[36] = int[36](
	// front
	0, 1, 2, 2, 3, 0,
	// right
	1, 5, 6, 6, 2, 1,
	// back
	7, 6, 5, 5, 4, 7,
	// left
	4, 0, 3, 3, 7, 4,
	// bottom
	4, 5, 1, 1, 0, 4,
	// top
	3, 2, 6, 6, 7, 3
);

void main()
{
	mat4 mvp = proj * view * inModelMatrices[gl_BaseInstance];

	int idx = indices[gl_VertexID];

	// vec4 posW = model * vec4(pos[idx], 1.0);
	// always center sky about camera
	// posW.xyz += cameraPos.xyz;
	// set z = w so that z/w = 1 (sky always on far plane)
	// gl_Position = (vp * posW).xyww;
	gl_Position = mvp * vec4(pos[idx], 1.0);

	// use local vertex position as cubemap lookup vector
	dir = pos[idx].xyz;
}
