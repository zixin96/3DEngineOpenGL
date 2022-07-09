#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <cstring>

// GCC compiler needs special treatment to make the struct tightly packed
#ifdef __GNUC__
#define PACKED_STRUCT __attribute__((packed,aligned(1)))
#else
#define PACKED_STRUCT
#endif

using glm::vec2;
using glm::vec4;

// tightly packed vec4 class with exactly 16 bytes
struct PACKED_STRUCT GpuVec4
{
	float x, y, z, w;

	GpuVec4() = default;

	explicit GpuVec4(float v): x(v), y(v), z(v), w(v)
	{
	}

	GpuVec4(float a, float b, float c, float d): x(a), y(b), z(c), w(d)
	{
	}

	explicit GpuVec4(const vec4& v): x(v.x), y(v.y), z(v.z), w(v.w)
	{
	}
};

// tightly packed mat4 class
struct PACKED_STRUCT GpuMat4
{
	float data[16];

	GpuMat4() = default;
	explicit GpuMat4(const glm::mat4& m) { memcpy(data, value_ptr(m), 16 * sizeof(float)); }
};
