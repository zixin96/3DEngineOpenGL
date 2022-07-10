#include "GLCanvas.h"

void GLCanvas::line(const vec3& p1, const vec3& p2, const vec4& c)
{
	mLines.push_back({.position = p1, .color = c});
	mLines.push_back({.position = p2, .color = c});
}

void GLCanvas::flush()
{
	if (mLines.empty())
		return;

	assert(mLines.size() < kMaxLines);

	glNamedBufferSubData(mMesh.mBufferVertices->getHandle(), 0, uint32_t(mLines.size() * sizeof(VertexData)), mLines.data());
	mProgLines.useProgram();
	mMesh.drawArrays(GL_LINES, 0, (GLint)mLines.size());

	mLines.clear();
}

void renderCameraFrustumGL(GLCanvas& canvas, const mat4& camView, const mat4& camProj, const vec4& color, int numSegments)
{
	using glm::normalize;

	const vec4 corners[] = {
		vec4(-1, -1, -1, 1), vec4(1, -1, -1, 1),
		vec4(1, 1, -1, 1), vec4(-1, 1, -1, 1),
		vec4(-1, -1, 1, 1), vec4(1, -1, 1, 1),
		vec4(1, 1, 1, 1), vec4(-1, 1, 1, 1)
	};

	vec3 pp[8];

	const mat4 invMVP = glm::inverse(camProj * camView);

	for (int i = 0; i < 8; i++)
	{
		const vec4 q = invMVP * corners[i];
		pp[i]        = glm::vec3(q) / q.w;
	}

	canvas.line(pp[0], pp[4], color);
	canvas.line(pp[1], pp[5], color);
	canvas.line(pp[2], pp[6], color);
	canvas.line(pp[3], pp[7], color);
	// near
	canvas.line(pp[0], pp[1], color);
	canvas.line(pp[1], pp[2], color);
	canvas.line(pp[2], pp[3], color);
	canvas.line(pp[3], pp[0], color);
	// x
	canvas.line(pp[0], pp[2], color);
	canvas.line(pp[1], pp[3], color);
	// far
	canvas.line(pp[4], pp[5], color);
	canvas.line(pp[5], pp[6], color);
	canvas.line(pp[6], pp[7], color);
	canvas.line(pp[7], pp[4], color);
	// x
	canvas.line(pp[4], pp[6], color);
	canvas.line(pp[5], pp[7], color);

	const float dimFactor = 0.7f;

	// bottom
	{
		vec3       p1 = pp[0];
		vec3       p2 = pp[1];
		const vec3 s1 = normalize(pp[4] - pp[0]);
		const vec3 s2 = normalize(pp[5] - pp[1]);
		for (int i = 0; i != numSegments; i++, p1 += s1, p2 += s2)
			canvas.line(p1, p2, color * dimFactor);
	}
	// top
	{
		vec3       p1 = pp[2];
		vec3       p2 = pp[3];
		const vec3 s1 = normalize(pp[6] - pp[2]);
		const vec3 s2 = normalize(pp[7] - pp[3]);
		for (int i = 0; i != numSegments; i++, p1 += s1, p2 += s2)
			canvas.line(p1, p2, color * dimFactor);
	}
	// left
	{
		vec3       p1 = pp[0];
		vec3       p2 = pp[3];
		const vec3 s1 = normalize(pp[4] - pp[0]);
		const vec3 s2 = normalize(pp[7] - pp[3]);
		for (int i = 0; i != numSegments; i++, p1 += s1, p2 += s2)
			canvas.line(p1, p2, color * dimFactor);
	}
	// right
	{
		vec3       p1 = pp[1];
		vec3       p2 = pp[2];
		const vec3 s1 = normalize(pp[5] - pp[1]);
		const vec3 s2 = normalize(pp[6] - pp[2]);
		for (int i = 0; i != numSegments; i++, p1 += s1, p2 += s2)
			canvas.line(p1, p2, color * dimFactor);
	}
}
