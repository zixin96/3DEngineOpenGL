#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "GLBuffer.h"
#include "GLProgram.h"
#include "GLShader.h"

#include "Util/Scene.h"

class GLImGui
{
public:
	GLImGui();
	~GLImGui();
	void render(int width, int height, const ImDrawData* draw_data);

private:
	void defaultInitImGui();

private:
	const GLchar* shaderCodeImGuiVertex = R"(
	#version 460 core
	layout (location = 0) in vec2 Position;
	layout (location = 1) in vec2 UV;
	layout (location = 2) in vec4 Color;

	layout (std140, binding = 7) uniform PerFrameData
	{
		mat4 mvp;
	};

	layout (location = 0) out vec2 Frag_UV;
	layout (location = 1) out vec4 Frag_Color;

	void main()
	{
		Frag_UV = UV;
		Frag_Color = Color;
		gl_Position = mvp * vec4(Position.xy, 0.0, 1.0);
	}
	)";

	const GLchar* shaderCodeImGuiFragment = R"(
	#version 460 core

	layout (location = 0) in vec2 Frag_UV;
	layout (location = 1) in vec4 Frag_Color;

	layout (location = 0) out vec4 out_Color;

	layout (binding = 0) uniform sampler2D Texture;

	void main()
	{
		out_Color = Frag_Color * texture(Texture, Frag_UV);
	}
	)";

	GLuint    mTexture            = 0;
	GLuint    mVAO                = 0;
	GLBuffer  mVertexBuffer       = GLBuffer(128 * 1024, nullptr, GL_DYNAMIC_STORAGE_BIT);
	GLBuffer  mElementBuffer      = GLBuffer(256 * 1024, nullptr, GL_DYNAMIC_STORAGE_BIT);
	GLShader  mVertexShader       = GLShader(GL_VERTEX_SHADER, shaderCodeImGuiVertex);
	GLShader  mFragmentShader     = GLShader(GL_FRAGMENT_SHADER, shaderCodeImGuiFragment);
	GLProgram mProgram            = GLProgram(mVertexShader, mFragmentShader);
	GLBuffer  mPerFrameDataBuffer = GLBuffer(sizeof(glm::mat4), nullptr, GL_DYNAMIC_STORAGE_BIT);
};

int  renderSceneTree(const Scene& scene, int node);
void imguiTextureWindowGL(const char* title, uint32_t texId);
