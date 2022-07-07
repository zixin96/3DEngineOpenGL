#version 460 core

#include <data/shaders/09CubeMap/GLBufferDecl.h>

struct PerVertex
{
    vec2 uv;
    vec3 normal;
    vec3 worldPos;
};

layout (location = 0) out PerVertex vtx;

vec3 getPosition(uint i)
{
    return vec3(inVertices[i].pos[0], inVertices[i].pos[1], inVertices[i].pos[2]);
}

vec3 getNormal(uint i)
{
    return vec3(inVertices[i].nor[0], inVertices[i].nor[1], inVertices[i].nor[2]);
}

vec2 getTexCoord(uint i)
{
    return vec2(inVertices[i].tc[0], inVertices[i].tc[1]);
}

void main()
{
    mat4 model = inModelMatrices[gl_BaseInstance];
    mat4 mvp = proj * view * model;

    // fetch index from storage buffer
	uint positionIndex = inIndices[gl_VertexID];

    vec3 pos = getPosition(positionIndex);
    gl_Position = mvp * vec4(pos, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    vtx.uv = getTexCoord(positionIndex);
    vtx.normal = normalMatrix * getNormal(positionIndex);
    vtx.worldPos = (model * vec4(pos, 1.0)).xyz;
}

