#include "GLMesh.h"
#include <glm/glm.hpp>
using glm::vec3;
using glm::vec2;

GLMesh::GLMesh(const MeshFileHeader& header,
               const Mesh*           meshes,
               const uint32_t*       indices,
               const float*          vertexData)
	: mNumIndices(header.indexDataSize / sizeof(uint32_t))
	, mIndexBuffer(header.indexDataSize, indices, 0)
	, mVertexBuffer(header.vertexDataSize, vertexData, 0)
// , mIndirectCommandBuffer(sizeof(DrawElementsIndirectCommand) * header.meshCount + sizeof(GLsizei),
//                          nullptr,
//                          GL_DYNAMIC_STORAGE_BIT)
{
	glCreateVertexArrays(1, &mVAO);
	glVertexArrayElementBuffer(mVAO, mIndexBuffer.getHandle());
	// glVertexArrayVertexBuffer(mVAO, 0, mVertexBuffer.getHandle(), 0, sizeof(vec3) + sizeof(vec3) + sizeof(vec2));
	glVertexArrayVertexBuffer(mVAO, 0, mVertexBuffer.getHandle(), 0, sizeof(vec3));
	// position
	glEnableVertexArrayAttrib(mVAO, 0);
	glVertexArrayAttribFormat(mVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(mVAO, 0, 0);
	// uv
	// glEnableVertexArrayAttrib(mVAO, 1);
	// glVertexArrayAttribFormat(mVAO, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vec3));
	// glVertexArrayAttribBinding(mVAO, 1, 0);
	// normal
	// glEnableVertexArrayAttrib(mVAO, 2);
	// glVertexArrayAttribFormat(mVAO, 2, 3, GL_FLOAT, GL_TRUE, sizeof(vec3) + sizeof(vec2));
	// glVertexArrayAttribBinding(mVAO, 2, 0);
	//
	// std::vector<uint8_t> drawCommands;
	//
	// const GLsizei numCommands = (GLsizei)header.meshCount;
	//
	// drawCommands.resize(sizeof(DrawElementsIndirectCommand) * numCommands + sizeof(GLsizei));
	//
	// // store the number of draw commands in the very beginning of the buffer
	// memcpy(drawCommands.data(), &numCommands, sizeof(numCommands));
	//
	// DrawElementsIndirectCommand* cmd = std::launder(
	//                                                 reinterpret_cast<DrawElementsIndirectCommand*>(drawCommands.data() + sizeof(GLsizei))
	//                                                );
	//
	// // prepare indirect commands buffer
	// for (uint32_t i = 0; i != numCommands; i++)
	// {
	// 	*cmd++ = {
	// 		.count_ = meshes[i].getLODIndicesCount(0),
	// 		.instanceCount_ = 1,
	// 		.firstIndex_ = meshes[i].indexOffset,
	// 		.baseVertex_ = meshes[i].vertexOffset,
	// 		.baseInstance_ = 0
	// 	};
	// }
	//
	// glNamedBufferSubData(mIndirectCommandBuffer.getHandle(), 0, drawCommands.size(), drawCommands.data());
	// glBindVertexArray(mVAO);
}

void GLMesh::draw(const MeshFileHeader& header) const
{
	glBindVertexArray(mVAO);
	// glBindBuffer(GL_DRAW_INDIRECT_BUFFER, mIndirectCommandBuffer.getHandle());
	// glBindBuffer(GL_PARAMETER_BUFFER, mIndirectCommandBuffer.getHandle());
	// glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*)sizeof(GLsizei), 0, (GLsizei)header.meshCount, 0);
	glDrawElements(GL_TRIANGLES,
	               static_cast<GLsizei>(mNumIndices),
	               GL_UNSIGNED_INT,
	               nullptr);
}

GLMesh::~GLMesh()
{
	glDeleteVertexArrays(1, &mVAO);
}
