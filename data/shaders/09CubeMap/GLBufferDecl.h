layout (std140, binding = 0) uniform PerFrameData
{
    mat4 model;
    mat4 vp;
    vec4 cameraPos;
};

// This struct should match vertex struct in c++ side
struct VertexData
{
    // use array to avoid paddings
    float pos[3];
    float nor[3];
    float tc[2];
};

// std430: an array of `float`s will match with a C++ array of `float`s
layout (std430, binding = 0) readonly buffer Vertices
{
    // Notice that this is an unbounded array 
    // where each element corresponids to exactly one vertex
    VertexData inVertices[];
};

layout(std430, binding = 1) readonly buffer Indices
{
	uint inIndices[];
};