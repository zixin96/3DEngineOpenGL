#include <string>
#include <vector>

#include "GpuMath.h"

enum MaterialFlags
{
	sMaterialFlags_CastShadow = 0x1,
	sMaterialFlags_ReceiveShadow = 0x2,
	sMaterialFlags_Transparent = 0x4,
};

constexpr uint64_t INVALID_TEXTURE = 0xFFFFFFFF;

struct PACKED_STRUCT MaterialData final
{
	GpuVec4 emissiveColor = {0.0f, 0.0f, 0.0f, 0.0f};
	GpuVec4 albedoColor   = {1.0f, 1.0f, 1.0f, 1.0f};
	// UV anisotropic roughness (isotropic lighting models use only the first value). ZW values are ignored
	GpuVec4 roughness          = {1.0f, 1.0f, 0.0f, 0.0f};
	float   transparencyFactor = 1.0f;
	float   alphaTest          = 0.0f;
	float   metallicFactor     = 0.0f;
	// flags that differ from material to material or from object to object
	uint32_t flags = sMaterialFlags_CastShadow | sMaterialFlags_ReceiveShadow;
	// maps
	uint64_t ambientOcclusionMap = INVALID_TEXTURE;
	uint64_t emissiveMap         = INVALID_TEXTURE;
	uint64_t albedoMap           = INVALID_TEXTURE;
	/// Occlusion (R), Roughness (G), Metallic (B) https://github.com/KhronosGroup/glTF/issues/857
	uint64_t metallicRoughnessMap = INVALID_TEXTURE;
	uint64_t normalMap            = INVALID_TEXTURE;
	uint64_t opacityMap           = INVALID_TEXTURE;
};

static_assert(sizeof(MaterialData) % 16 == 0, "MaterialDescription should be padded to 16 bytes");

void saveMaterials(const char* fileName, const std::vector<MaterialData>& materials, const std::vector<std::string>& files);
void loadMaterials(const char* fileName, std::vector<MaterialData>& materials, std::vector<std::string>& files);
