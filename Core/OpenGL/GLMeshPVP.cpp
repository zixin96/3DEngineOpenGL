#include "GLMeshPVP.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

static const GLuint kBufferIndex_Vertices = 0;

GLMeshPVP::GLMeshPVP(const uint32_t* indices,
                     uint32_t        indicesSize,
                     const float*    vertexData,
                     uint32_t        verticesSize)
	: mNumIndices(indicesSize / sizeof(uint32_t))
	, mBufferIndices(std::make_unique<GLBuffer>(indicesSize, indices, 0))
	, mBufferVertices(std::make_unique<GLBuffer>(verticesSize, vertexData, 0))
{
	glCreateVertexArrays(1, &mVao);
	glVertexArrayElementBuffer(mVao, mBufferIndices->getHandle());
}

GLMeshPVP::GLMeshPVP(const std::vector<uint32_t>& indices, const void* vertexData, uint32_t verticesSize)
	: mNumIndices((uint32_t)indices.size())
	, mBufferIndices(indices.size() ? std::make_unique<GLBuffer>(indices.size() * sizeof(uint32_t), indices.data(), 0) : nullptr)
	, mBufferVertices(std::make_unique<GLBuffer>(verticesSize, vertexData, GL_DYNAMIC_STORAGE_BIT))
{
	glCreateVertexArrays(1, &mVao);
	if (mBufferIndices)
		glVertexArrayElementBuffer(mVao, mBufferIndices->getHandle());
}

GLMeshPVP::GLMeshPVP(const char* fileName)
{
	std::vector<VertexData> vertices;
	std::vector<uint32_t>   indices;
	{
		const aiScene* scene = aiImportFile(fileName, aiProcess_Triangulate);
		if (!scene || !scene->HasMeshes())
		{
			printf("Unable to load '%s'\n", fileName);
			exit(255);
		}

		// assume we have one mesh in this scene file
		const aiMesh* mesh = scene->mMeshes[0];
		for (unsigned i = 0; i != mesh->mNumVertices; i++)
		{
			const aiVector3D v = mesh->mVertices[i];
			const aiVector3D n = mesh->mNormals[i];
			const aiVector3D t = mesh->mTextureCoords[0][i];
			vertices.push_back({.pos = vec3(v.x, v.y, v.z), .n = vec3(n.x, n.y, n.z), .tc = vec2(t.x, 1.0f - t.y)});
		}
		for (unsigned i = 0; i != mesh->mNumFaces; i++)
			for (unsigned j = 0; j != 3; j++)
				indices.push_back(mesh->mFaces[i].mIndices[j]);
		aiReleaseImport(scene);
	}
	const size_t kSizeIndices  = sizeof(uint32_t) * indices.size();
	const size_t kSizeVertices = sizeof(VertexData) * vertices.size();
	mNumIndices                = (uint32_t)indices.size();
	mBufferIndices             = std::make_unique<GLBuffer>((uint32_t)kSizeIndices, indices.data(), 0);
	mBufferVertices            = std::make_unique<GLBuffer>((uint32_t)kSizeVertices, vertices.data(), 0);
	glCreateVertexArrays(1, &mVao);
	glVertexArrayElementBuffer(mVao, mBufferIndices->getHandle());
}

void GLMeshPVP::draw() const
{
	drawElements(GL_TRIANGLES);
}

void GLMeshPVP::drawElements(GLenum mode) const
{
	glBindVertexArray(mVao);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_Vertices, mBufferVertices->getHandle());
	glDrawElements(mode, static_cast<GLsizei>(mNumIndices), GL_UNSIGNED_INT, nullptr);
}

void GLMeshPVP::drawArrays(GLenum mode, GLint first, GLint count)
{
	glBindVertexArray(mVao);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_Vertices, mBufferVertices->getHandle());
	glDrawArrays(mode, first, count);
}

GLMeshPVP::~GLMeshPVP()
{
	glDeleteVertexArrays(1, &mVao);
}
