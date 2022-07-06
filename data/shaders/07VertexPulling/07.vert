#version 460 core
layout (std140, binding = 0) uniform PerFrameData
{
	mat4 mvp;
};

// This struct should match vertex struct in c++ side
struct VertexData
{
    // use array to avoid paddings
    float pos[3];
    float tc[2];
};

// std430: an array of `float`s will match with a C++ array of `float`s
layout (std430, binding = 0) readonly buffer Vertices
{
    // Notice that this is an unbounded array 
    // where each element corresponids to exactly one vertex
    VertexData inVertices[];
};

vec3 getPosition(int i)
{
    return vec3(inVertices[i].pos[0], inVertices[i].pos[1], inVertices[i].pos[2]);
}

vec2 getTexCoord(int i)
{
    return vec2(inVertices[i].tc[0], inVertices[i].tc[1]);
}

layout (location = 0) out vec2 uv;

void main()
{
    vec3 pos = getPosition(gl_VertexID);
    gl_Position = mvp * vec4(pos, 1.0);
    uv = getTexCoord(gl_VertexID);
}

