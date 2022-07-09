#include "VtxData.h"
#include <cassert>
#include "UtilsMath.h"

MeshFileHeader loadMeshData(const char* meshFile, MeshData& out)
{
	MeshFileHeader header;

	FILE* f = fopen(meshFile, "rb");

	assert(f); // Did you forget to run "Ch5_Tool05_MeshConvert"?

	if (!f)
	{
		printf("Cannot open %s. Did you forget to run \"Ch5_Tool05_MeshConvert\"?\n", meshFile);
		exit(EXIT_FAILURE);
	}

	if (fread(&header, 1, sizeof(header), f) != sizeof(header))
	{
		printf("Unable to read mesh file header\n");
		exit(EXIT_FAILURE);
	}

	out.meshes.resize(header.meshCount);
	if (fread(out.meshes.data(), sizeof(Mesh), header.meshCount, f) != header.meshCount)
	{
		printf("Could not read mesh descriptors\n");
		exit(EXIT_FAILURE);
	}

	out.indexData.resize(header.indexDataSize / sizeof(uint32_t));
	out.vertexData.resize(header.vertexDataSize / sizeof(float));

	if ((fread(out.indexData.data(), 1, header.indexDataSize, f) != header.indexDataSize) ||
	    (fread(out.vertexData.data(), 1, header.vertexDataSize, f) != header.vertexDataSize))
	{
		printf("Unable to read index/vertex data\n");
		exit(255);
	}

	fclose(f);

	return header;
}

void saveMeshData(const char* fileName, const MeshData& m)
{
	FILE* f = fopen(fileName, "wb");

	const MeshFileHeader header = {
		.magicValue = 0x12345678,
		.meshCount = (uint32_t)m.meshes.size(),
		.dataBlockStartOffset = (uint32_t)(sizeof(MeshFileHeader) + m.meshes.size() * sizeof(Mesh)),
		.indexDataSize = (uint32_t)(m.indexData.size() * sizeof(uint32_t)),
		.vertexDataSize = (uint32_t)(m.vertexData.size() * sizeof(float))
	};

	fwrite(&header, 1, sizeof(header), f);
	fwrite(m.meshes.data(), sizeof(Mesh), header.meshCount, f);
	fwrite(m.indexData.data(), 1, header.indexDataSize, f);
	fwrite(m.vertexData.data(), 1, header.vertexDataSize, f);

	fclose(f);
}
