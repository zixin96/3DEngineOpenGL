#version 460 core

#include <data/shaders/13Tessellation/GLDecl.glsl>

// This struct should match vertex struct in c++ side
struct VertexData
{
    // use array to avoid paddings
    float pos[3];
    float tc[2];
};

layout (std430, binding = 0) restrict readonly buffer Vertices
{
    VertexData inVertices[];
};

layout(std430, binding = 1) restrict readonly buffer Matrices
{
	mat4 inModelMatrices[];
};

vec3 getPosition(int i)
{
    return vec3(inVertices[i].pos[0], inVertices[i].pos[1], inVertices[i].pos[2]);
}

vec2 getTexCoord(int i)
{
    return vec2(inVertices[i].tc[0], inVertices[i].tc[1]);
}

layout (location = 0) out vec2 uv_in;
layout (location = 1) out vec3 worldPos_in;

void main()
{
    mat4 MVP = proj * view * inModelMatrices[gl_DrawID];
    vec3 pos = getPosition(gl_VertexID);
    gl_Position = MVP * vec4(pos, 1.0);
    uv_in = getTexCoord(gl_VertexID);
    worldPos_in = (inModelMatrices[gl_DrawID] * vec4(pos, 1.0)).xyz;
}

