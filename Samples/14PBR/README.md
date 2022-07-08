# 14PBR

This demo shows how we can render a model using a PBR pipeline. The essence of PBR rendering in this demo consists of fetching the right textures (AO, MR, BRDF LUTs, etc) and apply them in a PBR fragment shader.

- The implementation of PBR fragment shader is based on https://github.com/KhronosGroup/glTF-WebGL-PBR/blob/master/shaders/pbr-frag.glsl
- AO, MR, Albedo, Normal textures are downloaded from glTF 2.0 sample models
- BRDF LUT and irradiance map are generated through external apps or DIY using Vulkan compute shader.
- Environment map is converted from an equirectangular map.
