#pragma once
#include "GLBuffer.h"
#include "Util/VtxData.h"

// struct DrawElementsIndirectCommand
// {
// 	GLuint count_;
// 	GLuint instanceCount_;
// 	GLuint firstIndex_;
// 	GLuint baseVertex_;
// 	GLuint baseInstance_;
// };

class GLMesh
{
public:
	GLMesh(const MeshFileHeader& header, const Mesh* meshes, const uint32_t* indices, const float* vertexData);
	void draw(const MeshFileHeader& header) const;
	~GLMesh();

	GLMesh(const GLMesh&) = delete;
	GLMesh(GLMesh&&)      = default;

private:
	GLuint   mVAO;
	uint32_t mNumIndices;

	GLBuffer mIndexBuffer;
	GLBuffer mVertexBuffer;
	// GLBuffer mIndirectCommandBuffer;
};
