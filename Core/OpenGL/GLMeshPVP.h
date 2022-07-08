#pragma once
#include "GLBuffer.h"
#include "Util/VtxData.h"

class GLMeshPVP
{
public:
	GLMeshPVP(const uint32_t* indices, uint32_t indicesSize, const float* vertexData, uint32_t verticesSize);
	void draw() const;
	~GLMeshPVP();

private:
	GLuint   mVao;
	uint32_t mNumIndices;

	GLBuffer mBufferIndices;
	GLBuffer mBufferVertices;
};
