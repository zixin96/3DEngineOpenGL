#pragma once

#include "GLMeshPVP.h"
#include "GLProgram.h"
#include "GLShader.h"

using glm::mat4;

class GLCanvas
{
public:
	void line(const vec3& p1, const vec3& p2, const vec4& c);
	void flush();
private:
	static constexpr uint32_t kMaxLines = 1024 * 1024;

	struct VertexData
	{
		vec3 position;
		vec4 color;
	};

	std::vector<VertexData> mLines;
	GLShader                mShdLinesVertex   = GLShader("data/shaders/lines.vert");
	GLShader                mShdLinesFragment = GLShader("data/shaders/lines.frag");
	GLProgram               mProgLines        = GLProgram(mShdLinesVertex, mShdLinesFragment);
	GLMeshPVP               mMesh             = GLMeshPVP({}, nullptr, sizeof(VertexData) * kMaxLines);
};

void renderCameraFrustumGL(GLCanvas& canvas, const mat4& camView, const mat4& camProj, const vec4& color, int numSegments = 1);
