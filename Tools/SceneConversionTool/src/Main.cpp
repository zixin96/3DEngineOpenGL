#include <assimp/cimport.h>
#include <assimp/material.h>
#include <assimp/GltfMaterial.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <rapidjson/istreamwrapper.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <filesystem>
#include <execution>
#include <fstream>

#include "meshoptimizer.h"
#include "stb_image_write.h"
#include "stb_image.h"
#include "stb_image_resize.h"

#include "Util/Material.h"
#include "Util/Scene.h"
#include "Util/Utils.h"
#include "Util/VtxData.h"

namespace fs = std::filesystem;

struct SceneConfig
{
	std::string fileName;
	std::string outputMesh;
	std::string outputScene;
	std::string outputMaterials;
	float       scale;
	bool        calculateLODs;
	bool        mergeInstances;
};

MeshData       gMeshData;
uint32_t       gIndexOffset        = 0;
uint32_t       gVertexOffset       = 0;
const uint32_t gNumElementsToStore = 3 + 3 + 2; // pos(vec3) + normal(vec3) + uv(vec2)

void makePrefix(int atLevel);

// conversion from aiMatrix4x4 to glm::mat4
glm::mat4 toMat4(const aiMatrix4x4& from);

void printMat4(const aiMatrix4x4& m);

std::string lowercaseString(const std::string& s);

// parses the json config file and returns a container of SceneConfig structures 
std::vector<SceneConfig> readConfigFile(const char* cfgFileName)
{
	std::ifstream ifs(cfgFileName);
	if (!ifs.is_open())
	{
		printf("Failed to load configuration file.\n");
		exit(EXIT_FAILURE);
	}

	rapidjson::IStreamWrapper    isw(ifs);
	rapidjson::Document          document;
	const rapidjson::ParseResult parseResult = document.ParseStream(isw);
	assert(!parseResult.IsError());
	assert(document.IsArray());

	std::vector<SceneConfig> configList;

	for (rapidjson::SizeType i = 0; i < document.Size(); i++)
	{
		configList.emplace_back(SceneConfig{
			                        .fileName = document[i]["input_scene"].GetString(),
			                        .outputMesh = document[i]["output_mesh"].GetString(),
			                        .outputScene = document[i]["output_scene"].GetString(),
			                        .outputMaterials = document[i]["output_materials"].GetString(),
			                        .scale = (float)document[i]["scale"].GetDouble(),
			                        .calculateLODs = document[i]["calculate_LODs"].GetBool(),
			                        .mergeInstances = document[i]["merge_instances"].GetBool()
		                        });
	}

	return configList;
}

// traverse() is a form of top-down recursive traversal
// where we create our implicit scene node objects in the Scene struct
void traverse(const aiScene* sourceScene,
              Scene&         scene,
              aiNode*        node,
              int            parent,
              int            atLevel)
{
	int newNodeID = addNode(scene, parent, atLevel);

	// if aiNode has a name, store it in our scene 
	if (node->mName.C_Str())
	{
		makePrefix(atLevel);
		printf("Node[%d].name = %s\n", newNodeID, node->mName.C_Str());

		uint32_t nameID = (uint32_t)scene.names.size();

		scene.names.push_back(std::string(node->mName.C_Str()));

		scene.nodeIDToNameID[newNodeID] = nameID;
	}

	// for each mesh attached in this node, create a sub-node for it
	for (size_t i = 0; i < node->mNumMeshes; i++)
	{
		int newSubNodeID = addNode(scene, newNodeID, atLevel + 1);;

		uint32_t nameID = (uint32_t)scene.names.size();

		scene.names.push_back(std::string(node->mName.C_Str()) + "_Mesh_" + std::to_string(i));

		scene.nodeIDToNameID[newSubNodeID] = nameID;

		int meshID = (int)node->mMeshes[i];

		scene.nodeIDToMeshID[newSubNodeID] = meshID;

		scene.nodeIDToMaterialID[newSubNodeID] = sourceScene->mMeshes[meshID]->mMaterialIndex;

		makePrefix(atLevel);
		printf("Node[%d].SubNode[%d].mesh     = %d\n", newNodeID, newSubNodeID, (int)meshID);

		makePrefix(atLevel);
		printf("Node[%d].SubNode[%d].material = %d\n",
		       newNodeID,
		       newSubNodeID,
		       sourceScene->mMeshes[meshID]->mMaterialIndex);

		// subnodes are only use to attach meshes, so set the local/global transform to identity
		scene.globalTransform[newSubNodeID] = glm::mat4(1.0f);
		scene.localTransform[newSubNodeID]  = glm::mat4(1.0f);
	}

	// global trans. is set to identity at the beginning of node conversion
	// it will be recalculated at the first frame or if the node is marked as changed. 
	scene.globalTransform[newNodeID] = glm::mat4(1.0f);

	// local trans. is fetched from aiNode
	scene.localTransform[newNodeID] = toMat4(node->mTransformation);

	if (node->mParent != nullptr)
	{
		makePrefix(atLevel);
		printf("\tNode[%d].parent         = %s\n", newNodeID, node->mParent->mName.C_Str());

		makePrefix(atLevel);
		printf("\tNode[%d].localTransform = ", newNodeID);
		printMat4(node->mTransformation);
		printf("\n");
	}

	for (unsigned int n = 0; n < node->mNumChildren; n++)
	{
		traverse(sourceScene, scene, node->mChildren[n], newNodeID, atLevel + 1);
	}
}

// retrieves the required parameters from aiMaterial structure
// and returns a MaterialData object that can be used with our GLSL shaders
// files: output a list of texture file names
// opacityMaps: output a list of textures that need to be combined with transparency maps
MaterialData convertAIMaterialToMaterialData(const aiMaterial*         M,
                                             std::vector<std::string>& files,
                                             std::vector<std::string>& opacityMaps)
{
	MaterialData D;

	aiColor4D Color;

	// extract individual color components
	if (aiGetMaterialColor(M, AI_MATKEY_COLOR_AMBIENT, &Color) == AI_SUCCESS)
	{
		D.emissiveColor = {Color.r, Color.g, Color.b, Color.a};
		if (D.emissiveColor.w > 1.0f) D.emissiveColor.w = 1.0f;
	}
	if (aiGetMaterialColor(M, AI_MATKEY_COLOR_DIFFUSE, &Color) == AI_SUCCESS)
	{
		D.albedoColor = {Color.r, Color.g, Color.b, Color.a};
		if (D.albedoColor.w > 1.0f) D.albedoColor.w = 1.0f;
	}
	if (aiGetMaterialColor(M, AI_MATKEY_COLOR_EMISSIVE, &Color) == AI_SUCCESS)
	{
		D.emissiveColor.x += Color.r;
		D.emissiveColor.y += Color.g;
		D.emissiveColor.z += Color.b;
		D.emissiveColor.w += Color.a;
		if (D.emissiveColor.w > 1.0f) D.albedoColor.w = 1.0f;
	}

	// assume that anything with an opaqueness of 95% or more is considered opaque and avoids any blending
	const float opaquenessThreshold = 0.05f;
	float       opacity             = 1.0f;

	// set material transparency factor based on opacity value
	if (aiGetMaterialFloat(M, AI_MATKEY_OPACITY, &opacity) == AI_SUCCESS)
	{
		D.transparencyFactor = glm::clamp(1.0f - opacity, 0.0f, 1.0f);
		if (D.transparencyFactor >= 1.0f - opaquenessThreshold) D.transparencyFactor = 0.0f;
		//? why not set alphaTest value here?
		// D.alphaTest = 0.5f;
	}

	// if the material contains a RGB transparency factor, use the max component value to calculate transparency factor
	if (aiGetMaterialColor(M, AI_MATKEY_COLOR_TRANSPARENT, &Color) == AI_SUCCESS)
	{
		const float transparency = std::max(std::max(Color.r, Color.g), Color.b);
		D.transparencyFactor     = glm::clamp(transparency, 0.0f, 1.0f);
		if (D.transparencyFactor >= 1.0f - opaquenessThreshold) D.transparencyFactor = 0.0f;
		D.alphaTest = 0.5f;
	}

	// fetch scalar properties of the material
	//!? Note: since assimp's pbrmaterial.h is deprecated, we use a different MACROs for metallic and rougness factor
	float tmp = 1.0f;
	if (aiGetMaterialFloat(M, AI_MATKEY_METALLIC_FACTOR, &tmp) == AI_SUCCESS)
	{
		D.metallicFactor = tmp;
	}
	if (aiGetMaterialFloat(M, AI_MATKEY_ROUGHNESS_FACTOR, &tmp) == AI_SUCCESS)
	{
		D.roughness = {tmp, tmp, tmp, tmp};
	}

	aiString         Path;
	aiTextureMapping Mapping;
	unsigned int     UVIndex           = 0;
	float            Blend             = 1.0f;
	aiTextureOp      TextureOp         = aiTextureOp_Add;
	aiTextureMapMode TextureMapMode[2] = {aiTextureMapMode_Wrap, aiTextureMapMode_Wrap};
	unsigned int     TextureFlags      = 0;

	if (aiGetMaterialTexture(M,
	                         aiTextureType_EMISSIVE,
	                         0,
	                         &Path,
	                         &Mapping,
	                         &UVIndex,
	                         &Blend,
	                         &TextureOp,
	                         TextureMapMode,
	                         &TextureFlags) == AI_SUCCESS)
	{
		D.emissiveMap = addUnique(files, Path.C_Str());
	}

	if (aiGetMaterialTexture(M,
	                         aiTextureType_DIFFUSE,
	                         0,
	                         &Path,
	                         &Mapping,
	                         &UVIndex,
	                         &Blend,
	                         &TextureOp,
	                         TextureMapMode,
	                         &TextureFlags) == AI_SUCCESS)
	{
		D.albedoMap = addUnique(files, Path.C_Str());

		const std::string albedoMap = std::string(Path.C_Str());

		if (albedoMap.find("grey_30") != albedoMap.npos)
		{
			D.flags |= sMaterialFlags_Transparent;
		}
	}

	// first try tangent space normal map
	if (aiGetMaterialTexture(M,
	                         aiTextureType_NORMALS,
	                         0,
	                         &Path,
	                         &Mapping,
	                         &UVIndex,
	                         &Blend,
	                         &TextureOp,
	                         TextureMapMode,
	                         &TextureFlags) == AI_SUCCESS)
	{
		D.normalMap = addUnique(files, Path.C_Str());
	}

	// if no classic normal map is present, we should check for height map texture.
	// , which can be converted into a normal map at a later stage of the conversion process
	if (D.normalMap == 0xFFFFFFFF)
	{
		if (aiGetMaterialTexture(M,
		                         aiTextureType_HEIGHT,
		                         0,
		                         &Path,
		                         &Mapping,
		                         &UVIndex,
		                         &Blend,
		                         &TextureOp,
		                         TextureMapMode,
		                         &TextureFlags) == AI_SUCCESS)
		{
			D.normalMap = addUnique(files, Path.C_Str());
		}
	}

	if (aiGetMaterialTexture(M,
	                         aiTextureType_OPACITY,
	                         0,
	                         &Path,
	                         &Mapping,
	                         &UVIndex,
	                         &Blend,
	                         &TextureOp,
	                         TextureMapMode,
	                         &TextureFlags) == AI_SUCCESS)
	{
		D.opacityMap = addUnique(opacityMaps, Path.C_Str());
		D.alphaTest  = 0.5f;
	}

	// The final step is to apply some heuristics for guessing the material's properties based on its name
	// These tricks may make our scene look better. 
	aiString    Name;
	std::string materialName;
	if (aiGetMaterialString(M, AI_MATKEY_NAME, &Name) == AI_SUCCESS)
	{
		materialName = Name.C_Str();
	}
	// apply heuristics
	if ((materialName.find("Glass") != std::string::npos) ||
	    (materialName.find("Vespa_Headlight") != std::string::npos))
	{
		D.alphaTest          = 0.75f;
		D.transparencyFactor = 0.1f;
		D.flags |= sMaterialFlags_Transparent;
	}
	else if (materialName.find("Bottle") != std::string::npos)
	{
		D.alphaTest          = 0.54f;
		D.transparencyFactor = 0.4f;
		D.flags |= sMaterialFlags_Transparent;
	}
	else if (materialName.find("Metal") != std::string::npos)
	{
		D.metallicFactor = 1.0f;
		D.roughness      = GpuVec4(0.1f, 0.1f, 0.0f, 0.0f);
	}

	return D;
}

// find a file in directory which "almost" coincides with the origFile (their lowercase versions coincide) 
std::string findSubstitute(const std::string& origFile)
{
	// Make absolute path
	auto apath = fs::absolute(fs::path(origFile));
	// Absolute lowercase filename [we compare with it]
	auto afile = lowercaseString(apath.filename().string());
	// Directory where in which the file should be
	auto dir = fs::path(origFile).remove_filename();

	// Iterate each file non-recursively and compare lowercase absolute path with 'afile'
	for (auto& p : fs::directory_iterator(dir))
		if (afile == lowercaseString(p.path().filename().string()))
			return p.path().string();

	return std::string{};
}

std::string fixTextureFile(const std::string& file)
{
	// TODO: check the findSubstitute() function
	return fs::exists(file) ? file : findSubstitute(file);
}

std::string lowercaseString(const std::string& s)
{
	// s.length() copies of a single white space
	std::string out(s.length(), ' ');
	std::transform(s.begin(), s.end(), out.begin(), tolower); // tolower is a C function that takes an uppercase alphabet and convert it to a lowercase character
	return out;
}

std::string replaceAll(const std::string& str, const std::string& oldSubStr, const std::string& newSubStr)
{
	std::string result = str;

	for (size_t p = result.find(oldSubStr); p != std::string::npos; p = result.find(oldSubStr))
		result.replace(p, oldSubStr.length(), newSubStr);

	return result;
}

// convert an existing texture file into our own runtime format as a new file
// and return the new file's name
std::string convertTexture(const std::string&                         file,
                           const std::string&                         basePath,
                           std::unordered_map<std::string, uint32_t>& opacityMapIndices,
                           const std::vector<std::string>&            opacityMaps)
{
	// all our output textures will have no more than 512x512 pixels
	const int maxNewWidth  = 512;
	const int maxNewHeight = 512;

	// use "/" as the path separators for cross-platform operations
	const auto srcFile = replaceAll(basePath + file, "\\", "/");

	// the new file name is "data/out_textures/srcFile__rescaled.png" where srcFile is
	// a source file name with all path separators replaced by double underscore
	const auto newFile = std::string("data/out_textures/") +
	                     lowercaseString(replaceAll(replaceAll(srcFile, "..", "__"), "/", "__") +
	                                     std::string("__rescaled")) + std::string(".png");

	// load this image (we force the loaded image to be in RGBA format to simplify texture handling code)
	int      texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(fixTextureFile(srcFile).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	uint8_t* src    = pixels;
	texChannels     = STBI_rgb_alpha;

	// a temporary dynamic array that contains a combined albedo and opacity map
	std::vector<uint8_t> tmpImage(maxNewWidth * maxNewHeight * 4);

	// if the texture failed to load, set our temp array as input data to avoid exiting here
	if (!src)
	{
		printf("Failed to load [%s] texture\n", srcFile.c_str());
		texWidth    = maxNewWidth;
		texHeight   = maxNewHeight;
		texChannels = STBI_rgb_alpha;
		src         = tmpImage.data();
	}
	else
	{
		printf("Loaded [%s] %dx%d texture with %d channels\n", srcFile.c_str(), texWidth, texHeight, texChannels);
	}

	// if this file has an associated opacity map,
	// load the opacity map and add its contents to the albedo map 
	if (opacityMapIndices.contains(file))
	{
		const auto opacityMapFile = replaceAll(basePath + opacityMaps[opacityMapIndices[file]], "\\", "/");

		int opacityWidth, opacityHeight;
		// opacity map is loaded as a simple gray scale image
		stbi_uc* opacityPixels = stbi_load(fixTextureFile(opacityMapFile).c_str(), &opacityWidth, &opacityHeight, nullptr, 1);

		if (!opacityPixels)
		{
			printf("Failed to load opacity mask [%s]\n", opacityMapFile.c_str());
		}

		// check the loaded image's validity
		assert(opacityPixels);
		assert(texWidth == opacityWidth);
		assert(texHeight == opacityHeight);

		// store the opacity mask in the alpha component of this image
		if (opacityPixels)
		{
			for (int y = 0; y != opacityHeight; y++)
			{
				for (int x = 0; x != opacityWidth; x++)
				{
					src[(y * opacityWidth + x) * texChannels + 3] = opacityPixels[y * opacityWidth + x];
				}
			}
		}

		stbi_image_free(opacityPixels);
	}

	// rescale the texture to our specified width and height
	const uint32_t       imgSize = texWidth * texHeight * texChannels;
	std::vector<uint8_t> mipData(imgSize);
	uint8_t*             dst = mipData.data();

	const int newW = std::min(texWidth, maxNewWidth);
	const int newH = std::min(texHeight, maxNewHeight);

	stbir_resize_uint8(src, texWidth, texHeight, 0, dst, newW, newH, 0, texChannels);

	stbi_write_png(newFile.c_str(), newW, newH, texChannels, dst, 0);

	if (pixels)
		stbi_image_free(pixels);

	return newFile;
}

// generate the internal file names for each of the textures
// and convert the contents of each texture into a GPU-compatible format
// materials: a list of material data
// basePath: an output directory for texture data
// files, opacityMaps: the container for all the texture files and opacity maps
void convertAndDownscaleAllTextures(const std::vector<MaterialData>& materials,
                                    const std::string&               basePath,
                                    std::vector<std::string>&        files,
                                    std::vector<std::string>&        opacityMaps)
{
	// each of the opacity maps is combined with the albedo map
	// , so we need to keep the correspondence between the opacity map list
	// and the global texture indices
	std::unordered_map<std::string, uint32_t> opacityMapIndices(files.size());

	for (const auto& m : materials)
	{
		// if both opacity map and albedo map are present,
		// associate this opacity map with the albedo map
		if (m.opacityMap != 0xFFFFFFFF && m.albedoMap != 0xFFFFFFFF)
		{
			opacityMapIndices[files[m.albedoMap]] = (uint32_t)m.opacityMap;
		}
	}

	// takes a source texture file name and returns a modified texture file name
	auto converter = [&](const std::string& s) -> std::string
	{
		return convertTexture(s, basePath, opacityMapIndices, opacityMaps);
	};

	// convert all of the texture files under parallel policy (new in C++ 17)
	std::transform(std::execution::par, std::begin(files), std::end(files), std::begin(files), converter);
}


void makePrefix(int atLevel)
{
	for (int i = 0; i < atLevel; i++) { printf("\t"); }
}

glm::mat4 toMat4(const aiMatrix4x4& from)
{
	mat4 mm;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			mm[i][j] = from[i][j];
		}
	}

	mat4 to;
	to[0][0] = (float)from.a1;
	to[0][1] = (float)from.b1;
	to[0][2] = (float)from.c1;
	to[0][3] = (float)from.d1;

	to[1][0] = (float)from.a2;
	to[1][1] = (float)from.b2;
	to[1][2] = (float)from.c2;
	to[1][3] = (float)from.d2;

	to[2][0] = (float)from.a3;
	to[2][1] = (float)from.b3;
	to[2][2] = (float)from.c3;
	to[2][3] = (float)from.d3;

	to[3][0] = (float)from.a4;
	to[3][1] = (float)from.b4;
	to[3][2] = (float)from.c4;
	to[3][3] = (float)from.d4;

	//TODO:  Check this implementation
	//TODO:  delete line 110-129 if this assert passes
	assert(to == mm);

	return mm;
}

void printMat4(const aiMatrix4x4& m)
{
	if (!m.IsIdentity())
	{
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				printf("%f ;", m[i][j]);
	}
	else
	{
		printf(" Identity");
	}
}

void processLods(std::vector<uint32_t>& indices, std::vector<float>& vertices, std::vector<std::vector<uint32_t>>& outLods)
{
	size_t verticesCountIn    = vertices.size() / 2;
	size_t targetIndicesCount = indices.size();

	uint8_t LOD = 1;

	printf("\n   LOD0: %i indices", int(indices.size()));

	outLods.push_back(indices);

	while (targetIndicesCount > 1024 && LOD < 8)
	{
		targetIndicesCount = indices.size() / 2;

		bool sloppy = false;

		size_t numOptIndices = meshopt_simplify(
		                                        indices.data(),
		                                        indices.data(), (uint32_t)indices.size(),
		                                        vertices.data(), verticesCountIn,
		                                        sizeof(float) * 3,
		                                        targetIndicesCount, 0.02f);

		// cannot simplify further
		if (static_cast<size_t>(numOptIndices * 1.1f) > indices.size())
		{
			if (LOD > 1)
			{
				// try harder
				numOptIndices = meshopt_simplifySloppy(
				                                       indices.data(),
				                                       indices.data(), indices.size(),
				                                       vertices.data(), verticesCountIn,
				                                       sizeof(float) * 3,
				                                       targetIndicesCount, 0.02f, nullptr);
				sloppy = true;
				if (numOptIndices == indices.size()) break;
			}
			else
				break;
		}

		indices.resize(numOptIndices);

		meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), verticesCountIn);

		printf("\n   LOD%i: %i indices %s", int(LOD), int(numOptIndices), sloppy ? "[sloppy]" : "");

		LOD++;

		outLods.push_back(indices);
	}
}

Mesh convertAIMesh(const aiMesh* m, const SceneConfig& cfg)
{
	const bool     hasTexCoords      = m->HasTextureCoords(0);
	const uint32_t streamElementSize = static_cast<uint32_t>(gNumElementsToStore * sizeof(float));

	Mesh result = {
		.streamCount = 1,
		.indexOffset = gIndexOffset,
		.vertexOffset = gVertexOffset,
		.vertexCount = m->mNumVertices,
		.streamOffset = {gVertexOffset * streamElementSize},
		.streamElementSize = {streamElementSize}
	};

	// Original data for LOD calculation
	std::vector<float>    srcVertices;
	std::vector<uint32_t> srcIndices;

	std::vector<std::vector<uint32_t>> outLods;

	auto& vertices = gMeshData.vertexData;

	for (size_t i = 0; i != m->mNumVertices; i++)
	{
		const aiVector3D v = m->mVertices[i];
		const aiVector3D n = m->mNormals[i];
		const aiVector3D t = hasTexCoords ? m->mTextureCoords[0][i] : aiVector3D();

		if (cfg.calculateLODs)
		{
			srcVertices.push_back(v.x);
			srcVertices.push_back(v.y);
			srcVertices.push_back(v.z);
		}

		vertices.push_back(v.x * cfg.scale);
		vertices.push_back(v.y * cfg.scale);
		vertices.push_back(v.z * cfg.scale);

		vertices.push_back(t.x);
		vertices.push_back(1.0f - t.y);

		vertices.push_back(n.x);
		vertices.push_back(n.y);
		vertices.push_back(n.z);
	}

	for (size_t i = 0; i != m->mNumFaces; i++)
	{
		if (m->mFaces[i].mNumIndices != 3)
			continue;
		for (unsigned j = 0; j != m->mFaces[i].mNumIndices; j++)
			srcIndices.push_back(m->mFaces[i].mIndices[j]);
	}

	if (!cfg.calculateLODs)
		outLods.push_back(srcIndices);
	else
		processLods(srcIndices, srcVertices, outLods);

	printf("\nCalculated LOD count: %u\n", (unsigned)outLods.size());

	uint32_t numIndices = 0;

	for (size_t l = 0; l < outLods.size(); l++)
	{
		for (size_t i = 0; i < outLods[l].size(); i++)
			gMeshData.indexData.push_back(outLods[l][i]);

		result.lodOffset[l] = numIndices;
		numIndices += (int)outLods[l].size();
	}

	result.lodOffset[outLods.size()] = numIndices;
	result.lodCount                  = (uint32_t)outLods.size();

	gIndexOffset += numIndices;
	gVertexOffset += m->mNumVertices;

	return result;
}

void processScene(const SceneConfig& cfg)
{
	// clear mesh data from previous scene
	gMeshData.meshes.clear();
	gMeshData.indexData.clear();
	gMeshData.vertexData.clear();

	gIndexOffset  = 0;
	gVertexOffset = 0;

	// extract base model path
	const std::size_t pathSeparator = cfg.fileName.find_last_of("/\\");
	const std::string basePath      = (pathSeparator != std::string::npos) ? cfg.fileName.substr(0, pathSeparator + 1) : std::string();

	const unsigned int flags = 0 |
	                           aiProcess_JoinIdenticalVertices |
	                           aiProcess_Triangulate |
	                           aiProcess_GenSmoothNormals |
	                           aiProcess_LimitBoneWeights |
	                           aiProcess_SplitLargeMeshes |
	                           aiProcess_ImproveCacheLocality |
	                           aiProcess_RemoveRedundantMaterials |
	                           aiProcess_FindDegenerates |
	                           aiProcess_FindInvalidData |
	                           aiProcess_GenUVCoords;

	printf("Loading scene from '%s'...\n", cfg.fileName.c_str());

	const aiScene* scene = aiImportFile(cfg.fileName.c_str(), flags);

	if (!scene || !scene->HasMeshes())
	{
		printf("Unable to load '%s'\n", cfg.fileName.c_str());
		exit(EXIT_FAILURE);
	}

	// 1. Mesh conversion as in Chapter 5
	gMeshData.meshes.reserve(scene->mNumMeshes);

	for (unsigned int i = 0; i != scene->mNumMeshes; i++)
	{
		printf("\nConverting meshes %u/%u...", i + 1, scene->mNumMeshes);
		Mesh mesh = convertAIMesh(scene->mMeshes[i], cfg);
		gMeshData.meshes.push_back(mesh);
	}

	saveMeshData(cfg.outputMesh.c_str(), gMeshData);

	Scene ourScene;

	// 2. Material conversion
	std::vector<MaterialData> materials;
	std::vector<std::string>& materialNames = ourScene.materialNames;

	std::vector<std::string> files;
	std::vector<std::string> opacityMaps;

	for (unsigned int m = 0; m < scene->mNumMaterials; m++)
	{
		aiMaterial* mm = scene->mMaterials[m];

		printf("Material [%s] %u\n", mm->GetName().C_Str(), m);
		materialNames.push_back(std::string(mm->GetName().C_Str()));

		MaterialData D = convertAIMaterialToMaterialData(mm, files, opacityMaps);
		materials.push_back(D);
		//dumpMaterial(files, D);
	}

	// 3. Texture processing, rescaling and packing
	convertAndDownscaleAllTextures(materials, basePath, files, opacityMaps);

	saveMaterials(cfg.outputMaterials.c_str(), materials, files);

	// 4. Scene hierarchy conversion
	traverse(scene, ourScene, scene->mRootNode, -1, 0);

	saveScene(cfg.outputScene.c_str(), ourScene);
}

int main()
{
	fs::create_directory("data/out_textures");

	const auto configs = readConfigFile("data/sceneconverter.json");

	for (const auto& cfg : configs)
	{
		processScene(cfg);
	}

	return 0;
}
