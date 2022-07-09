#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <vector>

using glm::vec3;
using glm::vec4;

namespace Math
{
	static constexpr float PI    = 3.14159265359f;
	static constexpr float TWOPI = 6.28318530718f;
}

struct BoundingBox
{
	vec3 min;
	vec3 max;
	BoundingBox() = default;

	BoundingBox(const vec3& min, const vec3& max) : min(glm::min(min, max)), max(glm::max(min, max))
	{
	}

	BoundingBox(const vec3* points, size_t numPoints)
	{
		vec3 vmin(std::numeric_limits<float>::max());
		vec3 vmax(std::numeric_limits<float>::lowest());

		for (size_t i = 0; i != numPoints; i++)
		{
			vmin = glm::min(vmin, points[i]);
			vmax = glm::max(vmax, points[i]);
		}
		min = vmin;
		max = vmax;
	}

	vec3 getSize() const { return vec3(max[0] - min[0], max[1] - min[1], max[2] - min[2]); }
	vec3 getCenter() const { return 0.5f * vec3(max[0] + min[0], max[1] + min[1], max[2] + min[2]); }

	void transform(const glm::mat4& t)
	{
		vec3 corners[] = {
			vec3(min.x, min.y, min.z),
			vec3(min.x, max.y, min.z),
			vec3(min.x, min.y, max.z),
			vec3(min.x, max.y, max.z),
			vec3(max.x, min.y, min.z),
			vec3(max.x, max.y, min.z),
			vec3(max.x, min.y, max.z),
			vec3(max.x, max.y, max.z),
		};
		for (auto& v : corners)
			v = vec3(t * vec4(v, 1.0f));
		*this = BoundingBox(corners, 8);
	}

	BoundingBox getTransformed(const glm::mat4& t) const
	{
		BoundingBox b = *this;
		b.transform(t);
		return b;
	}

	void combinePoint(const vec3& p)
	{
		min = glm::min(min, p);
		max = glm::max(max, p);
	}
};

template <typename T>
T clamp(T v, T a, T b)
{
	if (v < a) return a;
	if (v > b) return b;
	return v;
}

inline float random01()
{
	return (float)rand() / (float)RAND_MAX;
}

inline float randomFloat(float min, float max)
{
	return min + (max - min) * random01();
}

inline glm::vec3 randomVec(const glm::vec3& min, const glm::vec3& max)
{
	return glm::vec3(randomFloat(min.x, max.x), randomFloat(min.y, max.y), randomFloat(min.z, max.z));
}

inline glm::vec3 randVec()
{
	return randomVec(glm::vec3(-5, -5, -5), glm::vec3(5, 5, 5));
}

inline void getFrustumPlanes(glm::mat4 mvp, glm::vec4* planes)
{
	using glm::vec4;

	mvp       = glm::transpose(mvp);
	planes[0] = vec4(mvp[3] + mvp[0]); // left
	planes[1] = vec4(mvp[3] - mvp[0]); // right
	planes[2] = vec4(mvp[3] + mvp[1]); // bottom
	planes[3] = vec4(mvp[3] - mvp[1]); // top
	planes[4] = vec4(mvp[3] + mvp[2]); // near
	planes[5] = vec4(mvp[3] - mvp[2]); // far
}

inline void getFrustumCorners(glm::mat4 mvp, glm::vec4* points)
{
	using glm::vec4;

	const vec4 corners[] = {
		vec4(-1, -1, -1, 1), vec4(1, -1, -1, 1),
		vec4(1, 1, -1, 1), vec4(-1, 1, -1, 1),
		vec4(-1, -1, 1, 1), vec4(1, -1, 1, 1),
		vec4(1, 1, 1, 1), vec4(-1, 1, 1, 1)
	};

	const glm::mat4 invMVP = glm::inverse(mvp);

	for (int i = 0; i != 8; i++)
	{
		const vec4 q = invMVP * corners[i];
		points[i]    = q / q.w;
	}
}

inline bool isBoxInFrustum(glm::vec4* frustumPlanes, glm::vec4* frustumCorners, const BoundingBox& box)
{
	using glm::dot;
	using glm::vec4;

	for (int i = 0; i < 6; i++)
	{
		int r = 0;
		r += (dot(frustumPlanes[i], vec4(box.min.x, box.min.y, box.min.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(frustumPlanes[i], vec4(box.max.x, box.min.y, box.min.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(frustumPlanes[i], vec4(box.min.x, box.max.y, box.min.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(frustumPlanes[i], vec4(box.max.x, box.max.y, box.min.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(frustumPlanes[i], vec4(box.min.x, box.min.y, box.max.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(frustumPlanes[i], vec4(box.max.x, box.min.y, box.max.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(frustumPlanes[i], vec4(box.min.x, box.max.y, box.max.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (dot(frustumPlanes[i], vec4(box.max.x, box.max.y, box.max.z, 1.0f)) < 0.0) ? 1 : 0;
		if (r == 8) return false;
	}

	// check frustum outside/inside box
	int r = 0;
	r     = 0;
	for (int i = 0; i < 8; i++) r += ((frustumCorners[i].x > box.max.x) ? 1 : 0);
	if (r == 8) return false;
	r = 0;
	for (int i = 0; i < 8; i++) r += ((frustumCorners[i].x < box.min.x) ? 1 : 0);
	if (r == 8) return false;
	r = 0;
	for (int i = 0; i < 8; i++) r += ((frustumCorners[i].y > box.max.y) ? 1 : 0);
	if (r == 8) return false;
	r = 0;
	for (int i = 0; i < 8; i++) r += ((frustumCorners[i].y < box.min.y) ? 1 : 0);
	if (r == 8) return false;
	r = 0;
	for (int i = 0; i < 8; i++) r += ((frustumCorners[i].z > box.max.z) ? 1 : 0);
	if (r == 8) return false;
	r = 0;
	for (int i = 0; i < 8; i++) r += ((frustumCorners[i].z < box.min.z) ? 1 : 0);
	if (r == 8) return false;

	return true;
}

inline BoundingBox combineBoxes(const std::vector<BoundingBox>& boxes)
{
	std::vector<vec3> allPoints;
	allPoints.reserve(boxes.size() * 8);

	for (const auto& b : boxes)
	{
		allPoints.emplace_back(b.min.x, b.min.y, b.min.z);
		allPoints.emplace_back(b.min.x, b.min.y, b.max.z);
		allPoints.emplace_back(b.min.x, b.max.y, b.min.z);
		allPoints.emplace_back(b.min.x, b.max.y, b.max.z);

		allPoints.emplace_back(b.max.x, b.min.y, b.min.z);
		allPoints.emplace_back(b.max.x, b.min.y, b.max.z);
		allPoints.emplace_back(b.max.x, b.max.y, b.min.z);
		allPoints.emplace_back(b.max.x, b.max.y, b.max.z);
	}

	return BoundingBox(allPoints.data(), allPoints.size());
}
