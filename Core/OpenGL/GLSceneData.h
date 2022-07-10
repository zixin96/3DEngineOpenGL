#pragma once

#include "Util/Scene.h"
#include "Util/Material.h"
#include "Util/VtxData.h"
#include "GLShader.h"
#include "GLTexture.h"

class GLSceneData
{
public:
	GLSceneData(const char* meshFile,
	            const char* sceneFile,
	            const char* materialFile);

	std::vector<GLTexture> mAllMaterialTextures;

	MeshFileHeader mHeader;
	MeshData       mMeshData;

	Scene                     mScene;
	std::vector<MaterialData> mMaterials;
	std::vector<DrawData>     mShapes;

	void loadScene(const char* sceneFile);
};
