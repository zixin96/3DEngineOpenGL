#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <meshoptimizer.h>
#include "Util/VtxData.h"
#include "argh.h"

// should we output textual msgs during the conversion process for debugging purposes? 
bool gVerbose = true;

// the actual mesh descriptors and data are stored in a MeshData struct
MeshData gMeshData;

// two counters to track offsets of index and vertex mesh data inside the file
uint32_t gIndexOffset  = 0;
uint32_t gVertexOffset = 0;

// should we export texture coordinates and normal vectors?
bool gExportTextures = false;
bool gExportNormals  = false;

// by default, we export only the vertex position into the output file
uint32_t gNumElementsToStore = 3;

float gMeshScale = 0.01f;

Mesh ConvertAssimpMesh(const aiMesh* m)
{
	// assume there is a single LOD and all the vertex data is stored as a continuous data stream.
	// we also ignore all the material information and deal exclusively with the index and vertex data.

	const bool hasTexCoords = m->HasTextureCoords(0);

	// for each of the vertices, extract their data from the aiMesh object
	for (size_t i = 0; i != m->mNumVertices; i++)
	{
		const aiVector3D& v = m->mVertices[i];
		const aiVector3D& n = m->mNormals[i];
		const aiVector3D& t = hasTexCoords ? m->mTextureCoords[0][i] : aiVector3D();

		// apply a global mesh scale, o.w. mesh is too big
		gMeshData.vertexData.push_back(v.x * gMeshScale);
		gMeshData.vertexData.push_back(v.y * gMeshScale);
		gMeshData.vertexData.push_back(v.z * gMeshScale);

		if (gExportTextures)
		{
			gMeshData.vertexData.push_back(t.x);
			gMeshData.vertexData.push_back(1.0f - t.y);
		}

		if (gExportNormals)
		{
			gMeshData.vertexData.push_back(n.x);
			gMeshData.vertexData.push_back(n.y);
			gMeshData.vertexData.push_back(n.z);
		}
	}

	// for each of the faces, extract indices data
	for (size_t i = 0; i != m->mNumFaces; i++)
	{
		//!? skip if number of indices in this face is not equal to 3!
		//!? This will solve strange geometry bugs!
		if (m->mFaces[i].mNumIndices != 3) { continue; }

		const aiFace& f = m->mFaces[i];
		gMeshData.indexData.push_back(f.mIndices[0] + gVertexOffset);
		gMeshData.indexData.push_back(f.mIndices[1] + gVertexOffset);
		gMeshData.indexData.push_back(f.mIndices[2] + gVertexOffset);
	}

	const uint32_t numElements       = gNumElementsToStore;
	const uint32_t streamElementSize = static_cast<uint32_t>(numElements * sizeof(float));
	const uint32_t numIndices        = m->mNumFaces * 3;
	const uint32_t meshSize          = static_cast<uint32_t>(m->mNumVertices * streamElementSize + numIndices * sizeof(uint32_t));

	const Mesh result = {
		.lodCount = 1,
		.streamCount = 1,
		.materialID = 0, // we are not yet exporting materials
		.meshSize = meshSize,
		.vertexCount = m->mNumVertices,
		.lodOffset = {
			gIndexOffset * sizeof(uint32_t),
			(gIndexOffset + numIndices) * sizeof(uint32_t)
		},
		.streamOffset = {gVertexOffset * streamElementSize},
		.streamElementSize = {streamElementSize}
	};

	// after processing this mesh, increment offset counters for the next mesh
	gIndexOffset += numIndices;
	gVertexOffset += m->mNumVertices;
	return result;
}

// load the scene and convert each mesh into our internal format
bool loadFile(const char* fileName)
{
	if (gVerbose)
	{
		printf("Loading '%s'...\n", fileName);
	}

	// specify desired flags for ASSIMP to process
	// Notably, all transformation hierarchies are flattened
	// and the resulting transformation matrices are applied to mesh vertices
	const unsigned int flags = aiProcess_JoinIdenticalVertices |
	                           aiProcess_Triangulate |
	                           aiProcess_GenSmoothNormals |
	                           aiProcess_PreTransformVertices |
	                           aiProcess_RemoveRedundantMaterials |
	                           aiProcess_FindDegenerates |
	                           aiProcess_FindInvalidData |
	                           aiProcess_FindInstances |
	                           aiProcess_OptimizeMeshes;

	const aiScene* scene = aiImportFile(fileName, flags);

	if (!scene || !scene->HasMeshes())
	{
		printf("Unable to load '%s'\n", fileName);
		exit(255);
	}

	// resize the mesh descriptor container
	gMeshData.meshes.reserve(scene->mNumMeshes);
	// convert each mesh to our internal representation
	for (size_t i = 0; i != scene->mNumMeshes; i++)
	{
		gMeshData.meshes.push_back(ConvertAssimpMesh(scene->mMeshes[i]));
	}

	return true;
}

void saveMeshesToFile(FILE* f)
{
	const MeshFileHeader header = {
		.magicValue = 0x12345678,
		.meshCount = static_cast<uint32_t>(gMeshData.meshes.size()),
		.dataBlockStartOffset = static_cast<uint32_t>(sizeof(MeshFileHeader) + gMeshData.meshes.size() * sizeof(Mesh)),
		.indexDataSize = static_cast<uint32_t>(gMeshData.indexData.size() * sizeof(uint32_t)),
		.vertexDataSize = static_cast<uint32_t>(gMeshData.vertexData.size() * sizeof(float))
	};

	fwrite(&header, 1, sizeof(header), f);
	fwrite(gMeshData.meshes.data(), sizeof(Mesh), header.meshCount, f);
	fwrite(gMeshData.indexData.data(), 1, header.indexDataSize, f);
	fwrite(gMeshData.vertexData.data(), 1, header.vertexDataSize, f);
}

int main(int argc, char** argv)
{
	argh::parser cmdl(argv);

	if (cmdl.size() < 3)
	{
		printf("Usage: meshconvert <input> <output> [--export-texcoords | -t] [--export-normals | -n]\n");
		printf("Options: \n");
		printf("\t--export-texcoords | -t: export texture coordinates\n");
		printf("\t--export-normals | -n: export normals\n");
		exit(255);
	}

	if (cmdl[{"-n", "---export-normals"}])
	{
		gExportNormals = true;
	}

	if (cmdl[{"-t", "---export-texcoords"}])
	{
		gExportTextures = true;
	}

	if (gExportTextures) { gNumElementsToStore += 2; }
	if (gExportNormals) { gNumElementsToStore += 3; }

	if (!loadFile(cmdl[1].c_str()))
	{
		exit(255);
	}

	FILE* f = fopen(cmdl[2].c_str(), "wb");
	saveMeshesToFile(f);
	fclose(f);
	return 0;
}
