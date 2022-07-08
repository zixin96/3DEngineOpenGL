#include "GLMeshPVP.h"

GLMeshPVP::GLMeshPVP(const uint32_t* indices,
                     uint32_t        indicesSize,
                     const float*    vertexData,
                     uint32_t        verticesSize)
	: mNumIndices(indicesSize / sizeof(uint32_t))
	, mBufferIndices(indicesSize, indices, 0)
	, mBufferVertices(verticesSize, vertexData, 0)
{
	glCreateVertexArrays(1, &mVao);
	glVertexArrayElementBuffer(mVao, mBufferIndices.getHandle());
}

void GLMeshPVP::draw() const
{
	glBindVertexArray(mVao);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mBufferVertices.getHandle());
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mNumIndices), GL_UNSIGNED_INT, nullptr);
}

GLMeshPVP::~GLMeshPVP()
{
	glDeleteVertexArrays(1, &mVao);
}
