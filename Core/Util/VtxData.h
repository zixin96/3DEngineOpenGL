#pragma once
#include <cstdint>
#include <vector>

// define the limits on how many LODs and vertex streams we can have in a single mesh
constexpr uint32_t MAX_LODS    = 8;
constexpr uint32_t MAX_STREAMS = 8;

struct Mesh final
{
	uint32_t lodCount;

	uint32_t streamCount;

	// an abstract ID that allows us to reference any material data that is stored else where
	uint32_t materialID;

	// size of the mesh is the sum of all LOD index array sizes and the sum of all individual stream sizes
	uint32_t meshSize;

	// vertex count is the total # of vertices in this mesh
	uint32_t vertexCount;

	// this array stores the offsets to the beginning of each LOD.
	// this array contains one extra item at the end, which serves as a marker to calculate the size of the last LOD
	uint32_t lodOffset[MAX_LODS] = {0};

	// store the offset for each stream
	uint32_t streamOffset[MAX_STREAMS] = {0};

	// store element size for each stream: e.g. Vertex only has size 3, Vertex + TexCoord has size 6
	uint32_t streamElementSize[MAX_STREAMS] = {0};

	// calculate the sizes of each LOD
	uint32_t getLODIndicesCount(uint32_t lod) const { return lodOffset[lod + 1] - lodOffset[lod]; }
};

// our mesh data file begins with a simple header to allows for the rapid fetching of the mesh list
struct MeshFileHeader
{
	// a magic value is stored in the first 4 bytes of the header to ensure data integrity and to check the validity of the header
	uint32_t magicValue;

	// the # of different meshes in this file
	uint32_t meshCount;

	// store an offset to the beginning of the mesh data
	uint32_t dataBlockStartOffset;

	// store the sizes of index and vertex data in bytes
	uint32_t indexDataSize;
	uint32_t vertexDataSize;
};

struct MeshData
{
	std::vector<Mesh> meshes;
	// TODO: you could combine index and vertex data into a single large byte buffer
	std::vector<uint32_t> indexData;
	std::vector<float>    vertexData;
};

MeshFileHeader loadMeshData(const char* meshFile, MeshData& out);
