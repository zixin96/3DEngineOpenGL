#version 460 core

#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : enable

#include <data/shaders/15LargeScene/material.glsl>

layout(std140, binding = 0) uniform PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

layout(std430, binding = 2) restrict readonly buffer Materials
{
	MaterialData in_Materials[];
};

layout (location=0) in vec2 v_tc;
layout (location=1) in vec3 v_worldNormal;
layout (location=2) in vec3 v_worldPos;
layout (location=3) in flat uint matIdx;

layout (location=0) out vec4 out_FragColor;

layout (binding = 5) uniform samplerCube texEnvMap;
layout (binding = 6) uniform samplerCube texEnvMapIrradiance;
layout (binding = 7) uniform sampler2D texBRDF_LUT;

#include <data/shaders/15LargeScene/alphaTest.glsl>
#include <data/shaders/15LargeScene/PBR.glsl>

void main()
{
	MaterialData mtl = in_Materials[matIdx];

	vec4 albedo = mtl.albedoColor;
	vec3 normalSample = vec3(0.0, 0.0, 0.0);

	// Sampling Bindless Textures, page 340, OpenGL Red Book
	// Handles as intergers, https://www.khronos.org/opengl/wiki/Bindless_Texture
	// unpackUint2x32, https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_gpu_shader_int64.txt

	// fetch albedo
	if (mtl.albedoMap > 0)
		albedo = texture( sampler2D(unpackUint2x32(mtl.albedoMap)), v_tc); 
	if (mtl.normalMap > 0)
		normalSample = texture( sampler2D(unpackUint2x32(mtl.normalMap)), v_tc).xyz;

	runAlphaTest(albedo.a, mtl.alphaTest);

	// world-space normal
	vec3 n = normalize(v_worldNormal);

	// normal mapping: skip missing normal maps
	if (length(normalSample) > 0.5)
		n = perturbNormal(n, normalize(cameraPos.xyz - v_worldPos.xyz), normalSample, v_tc);

	vec3 lightDir = normalize(vec3(-1.0, 1.0, 0.1));

	float NdotL = clamp( dot( n, lightDir ), 0.3, 1.0 );

	out_FragColor = vec4( albedo.rgb * NdotL, 1.0 );
};
