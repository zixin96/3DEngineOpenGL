#include "Material.h"
#include "Scene.h"

void saveMaterials(const char*                             fileName,
                   const std::vector<MaterialData>& materials,
                   const std::vector<std::string>&         files)
{
	FILE* f = fopen(fileName, "wb");
	if (!f)
		return;

	uint32_t sz = (uint32_t)materials.size();
	fwrite(&sz, 1, sizeof(uint32_t), f);
	fwrite(materials.data(), sizeof(MaterialData), sz, f);
	saveStringList(f, files);
	fclose(f);
}

void loadMaterials(const char*                       fileName,
                   std::vector<MaterialData>& materials,
                   std::vector<std::string>&         files)
{
	FILE* f = fopen(fileName, "rb");
	if (!f)
	{
		printf("Cannot load file %s\nPlease run SceneConverter tool from Chapter7\n", fileName);
		exit(255);
	}

	uint32_t sz;
	fread(&sz, 1, sizeof(uint32_t), f);
	materials.resize(sz);
	fread(materials.data(), sizeof(MaterialData), materials.size(), f);
	loadStringList(f, files);
	fclose(f);
}
