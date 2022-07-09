// A similar struct can be found in C++
struct MaterialData
{
	vec4 emissiveColor;
	vec4 albedoColor;
	vec4 roughness;

	float transparencyFactor;
	float alphaTest;
	float metallicFactor;

	uint  flags;

	uint64_t ambientOcclusionMap;
	uint64_t emissiveMap;
	uint64_t albedoMap;
	uint64_t metallicRoughnessMap;
	uint64_t normalMap;
	uint64_t opacityMap;
};
