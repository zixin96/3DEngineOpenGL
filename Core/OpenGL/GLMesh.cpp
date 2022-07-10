#include "GLMesh.h"
#include <glm/glm.hpp>
using glm::vec3;
using glm::vec2;

const static GLuint kBufferIndex_ModelMatrices = 1;
const static GLuint kBufferIndex_Materials     = 2;

GLMesh::GLMesh(const GLSceneData& data)
	: mNumIndices(data.mHeader.indexDataSize / sizeof(uint32_t))
	, mBufferIndices(data.mHeader.indexDataSize, data.mMeshData.indexData.data(), 0)
	, mBufferVertices(data.mHeader.vertexDataSize, data.mMeshData.vertexData.data(), 0)
	, mBufferMaterials(sizeof(MaterialData) * data.mMaterials.size(), data.mMaterials.data(), 0)
	  // Indirect buffer contains: NumberOfDrawCommands + Commands, where NumberOfDrawCommands is represented by one GLsizei
	, mBufferIndirect(sizeof(DrawElementsIndirectCommand) * data.mShapes.size() + sizeof(GLsizei), nullptr, GL_DYNAMIC_STORAGE_BIT)
	, mBufferModelMatrices(sizeof(glm::mat4) * data.mShapes.size(), nullptr, GL_DYNAMIC_STORAGE_BIT)
{
	glCreateVertexArrays(1, &mVao);
	glVertexArrayElementBuffer(mVao, mBufferIndices.getHandle());
	glVertexArrayVertexBuffer(mVao, 0, mBufferVertices.getHandle(), 0, sizeof(vec3) + sizeof(vec3) + sizeof(vec2));
	// position
	glEnableVertexArrayAttrib(mVao, 0);
	glVertexArrayAttribFormat(mVao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(mVao, 0, 0);
	// uv
	glEnableVertexArrayAttrib(mVao, 1);
	glVertexArrayAttribFormat(mVao, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vec3));
	glVertexArrayAttribBinding(mVao, 1, 0);
	// normal
	glEnableVertexArrayAttrib(mVao, 2);
	glVertexArrayAttribFormat(mVao, 2, 3, GL_FLOAT, GL_FALSE, sizeof(vec3) + sizeof(vec2)); //? Book says GL_TRUE, however, GL_TRUE only applies if type is an integer type
	glVertexArrayAttribBinding(mVao, 2, 0);

	std::vector<uint8_t> drawCommands;

	drawCommands.resize(sizeof(DrawElementsIndirectCommand) * data.mShapes.size() + sizeof(GLsizei));

	// store the number of draw commands in the very beginning of the buffer
	const GLsizei numCommands = (GLsizei)data.mShapes.size();
	memcpy(drawCommands.data(), &numCommands, sizeof(numCommands));

	// obtain a pointer that points to where we should insert the draw commands
	DrawElementsIndirectCommand* cmd = std::launder(reinterpret_cast<DrawElementsIndirectCommand*>(
		                                                drawCommands.data() + sizeof(GLsizei)
	                                                ));

	// prepare indirect commands buffer
	for (size_t i = 0; i != data.mShapes.size(); i++)
	{
		const uint32_t meshIdx = data.mShapes[i].meshIndex;
		const uint32_t lod     = data.mShapes[i].LOD;
		*cmd++                 = {
			.count = data.mMeshData.meshes[meshIdx].getLODIndicesCount(lod),
			.instanceCount = 1,
			.firstIndex = data.mShapes[i].indexOffset,
			.baseVertex = data.mShapes[i].vertexOffset,
			.baseInstance = data.mShapes[i].materialIndex
		};
	}

	glNamedBufferSubData(mBufferIndirect.getHandle(), 0, drawCommands.size(), drawCommands.data());

	std::vector<glm::mat4> matrices(data.mShapes.size());
	size_t                 i = 0;
	for (const auto& c : data.mShapes)
		matrices[i++] = data.mScene.globalTransform[c.transformIndex];

	glNamedBufferSubData(mBufferModelMatrices.getHandle(), 0, matrices.size() * sizeof(mat4), matrices.data());
}

void GLMesh::draw(const GLSceneData& data) const
{
	glBindVertexArray(mVao);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_ModelMatrices, mBufferModelMatrices.getHandle());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_Materials, mBufferMaterials.getHandle());

	// upload the command container
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, mBufferIndirect.getHandle());
	glBindBuffer(GL_PARAMETER_BUFFER, mBufferIndirect.getHandle());

	glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*)sizeof(GLsizei), 0, (GLsizei)data.mShapes.size(), 0);
}

GLMesh::~GLMesh()
{
	glDeleteVertexArrays(1, &mVao);
}
