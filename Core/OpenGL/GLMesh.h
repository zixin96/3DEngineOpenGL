#pragma once
#include "GLBuffer.h"
#include "GLSceneData.h"
#include "Util/VtxData.h"

// describes a single draw command
struct DrawElementsIndirectCommand
{
	GLuint count;
	GLuint instanceCount;
	GLuint firstIndex;
	GLuint baseVertex;
	GLuint baseInstance;
};

class GLMesh final
{
public:
	explicit GLMesh(const GLSceneData& data);

	void draw(const GLSceneData& data) const;

	~GLMesh();

	GLMesh(const GLMesh&);
	GLMesh(GLMesh&&);

private:
	GLuint   mVao;
	uint32_t mNumIndices;

	GLBuffer mBufferIndices;
	GLBuffer mBufferVertices;
	GLBuffer mBufferMaterials;

	GLBuffer mBufferIndirect;

	GLBuffer mBufferModelMatrices;
};
