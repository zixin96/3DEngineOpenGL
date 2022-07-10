#include "GLSceneData.h"

// convert a texture index into an OpenGL bindless texture handle
static uint64_t getTextureHandleBindless(uint64_t idx, const std::vector<GLTexture>& textures)
{
	if (idx == INVALID_TEXTURE) return 0;

	return textures[idx].getHandleBindless();
}

GLSceneData::GLSceneData(
	const char* meshFile,
	const char* sceneFile,
	const char* materialFile)
{
	// load mesh data
	mHeader = loadMeshData(meshFile, mMeshData);

	// load scene data
	loadScene(sceneFile);

	// load material data
	std::vector<std::string> textureFiles;
	loadMaterials(materialFile, mMaterials, textureFiles);

	for (const auto& f : textureFiles)
	{
		mAllMaterialTextures.emplace_back(GL_TEXTURE_2D, f.c_str());
	}

	for (auto& mtl : mMaterials)
	{
		mtl.ambientOcclusionMap  = getTextureHandleBindless(mtl.ambientOcclusionMap, mAllMaterialTextures);
		mtl.emissiveMap          = getTextureHandleBindless(mtl.emissiveMap, mAllMaterialTextures);
		mtl.albedoMap            = getTextureHandleBindless(mtl.albedoMap, mAllMaterialTextures);
		mtl.metallicRoughnessMap = getTextureHandleBindless(mtl.metallicRoughnessMap, mAllMaterialTextures);
		mtl.normalMap            = getTextureHandleBindless(mtl.normalMap, mAllMaterialTextures);
	}
}

void GLSceneData::loadScene(const char* sceneFile)
{
	::loadScene(sceneFile, mScene);

	// prepare draw data buffer
	for (const auto& c : mScene.nodeIDToMeshID)
	{
		auto material = mScene.nodeIDToMaterialID.find(c.first);
		if (material != mScene.nodeIDToMaterialID.end())
		{
			mShapes.push_back(DrawData{
				                  .meshIndex = c.second,
				                  .materialIndex = material->second,
				                  .LOD = 0,
				                  .indexOffset = mMeshData.meshes[c.second].indexOffset,
				                  .vertexOffset = mMeshData.meshes[c.second].vertexOffset,
				                  .transformIndex = c.first
			                  });
		}
	}

	// force recalculation of all global transformations
	markAsChanged(mScene, 0);
	recalculateGlobalTransforms(mScene);
}
