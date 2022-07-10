#pragma once
#include <memory>

#include "GLBuffer.h"
#include "Util/VtxData.h"

using glm::vec2;

struct VertexData
{
	vec3 pos;
	vec3 n;
	vec2 tc;
};

class GLMeshPVP
{
public:
	GLMeshPVP(const uint32_t* indices, uint32_t indicesSize, const float* vertexData, uint32_t verticesSize);
	GLMeshPVP(const std::vector<uint32_t>& indices, const void* vertexData, uint32_t verticesSize);
	explicit GLMeshPVP(const char* fileName);

	void draw() const;
	void drawElements(GLenum mode = GL_TRIANGLES) const;
	void drawArrays(GLenum mode, GLint first, GLint count);

	~GLMeshPVP();


	GLuint   mVao;
	uint32_t mNumIndices;

	std::unique_ptr<GLBuffer> mBufferIndices;
	std::unique_ptr<GLBuffer> mBufferVertices;
};
