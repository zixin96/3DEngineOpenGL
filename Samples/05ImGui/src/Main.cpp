#define GLFW_INCLUDE_NONE // with this line, glad and glfw3 could be included in any order
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "OpenGL/GLApp.h"
using glm::mat4;
using glm::vec3;
using glm::vec4;

#include <iostream>
using std::cout;
using std::endl;

#include <imgui/imgui.h>


static const char* shaderCodeVertexImGui = R"(
	#version 460 core
	layout (location = 0) in vec2 Position;
	layout (location = 1) in vec2 UV;
	layout (location = 2) in vec4 Color;

	layout (std140, binding = 0) uniform PerFrameData
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

static const char* shaderCodeFragmentImGui = R"(
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

int main()
{
	GLApp app;

	// set a callback for key events
	glfwSetKeyCallback(app.getWindow(), [](GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		// press escape key will close the window
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	});

	glfwSetCursorPosCallback(app.getWindow(), [](auto* window, double x, double y)
	{
		// enable ImGUI mouse interaction
		ImGui::GetIO().MousePos = ImVec2((float)x, (float)y);
	});

	glfwSetMouseButtonCallback(app.getWindow(), [](auto* window, int button, int action, int mods)
	{
		// enable ImGUI mouse interaction
		auto&     io      = ImGui::GetIO();
		const int idx     = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
		io.MouseDown[idx] = action == GLFW_PRESS;
	});

	// ImGui Initialization
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	// Configure Imgui font
	ImFontConfig cfg         = ImFontConfig();
	cfg.FontDataOwnedByAtlas = false;
	cfg.RasterizerMultiply   = 1.5f;          // brighten up the font to make it more readable
	cfg.SizePixels           = 1000.f / 50.f; // This should be window height / the desired number of text lines to be fit in the window
	// improve text appearance
	cfg.PixelSnapH  = true;
	cfg.OversampleH = 4;
	cfg.OversampleV = 4;
	ImFont* Font    = io.Fonts->AddFontFromFileTTF("data/Inconsolata-Light.ttf", cfg.SizePixels, &cfg);

	// Take the font atlas bitmap created by Imgui and use it to create an OpenGL texture
	unsigned char* pixels = nullptr;
	int            w, h;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureStorage2D(texture, 1, GL_RGBA8, w, h);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTextureSubImage2D(texture, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTextures(0, 1, &texture);

	// pass texture handle to imgui
	io.Fonts->TexID            = (ImTextureID)(intptr_t)texture;
	io.FontDefault             = Font;
	io.DisplayFramebufferScale = ImVec2(1, 1);

	// Create VAO, VBO, EBO to render geometry data coming from ImGui
	GLuint VAOImGui;
	GLuint VBOImGui;
	GLuint EBOImGui;

	glCreateVertexArrays(1, &VAOImGui);

	// use an upper limit of 256KB for the indices and vertices data

	glCreateBuffers(1, &VBOImGui);
	glNamedBufferStorage(VBOImGui, 256 * 1024, nullptr, GL_DYNAMIC_STORAGE_BIT);

	glCreateBuffers(1, &EBOImGui);
	glNamedBufferStorage(EBOImGui, 256 * 1024, nullptr, GL_DYNAMIC_STORAGE_BIT);

	glVertexArrayElementBuffer(VAOImGui, EBOImGui);

	// setup vertex attributes

	glVertexArrayVertexBuffer(VAOImGui, 0, VBOImGui, 0, sizeof(ImDrawVert)); // ImDrawVert is the Imgui's vertex data

	glEnableVertexArrayAttrib(VAOImGui, 0);
	glEnableVertexArrayAttrib(VAOImGui, 1);
	glEnableVertexArrayAttrib(VAOImGui, 2);

	glVertexArrayAttribFormat(VAOImGui, 0, 2, GL_FLOAT, GL_FALSE, IM_OFFSETOF(ImDrawVert, pos));
	glVertexArrayAttribFormat(VAOImGui, 1, 2, GL_FLOAT, GL_FALSE, IM_OFFSETOF(ImDrawVert, uv));
	glVertexArrayAttribFormat(VAOImGui, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, IM_OFFSETOF(ImDrawVert, col));

	glVertexArrayAttribBinding(VAOImGui, 0, 0);
	glVertexArrayAttribBinding(VAOImGui, 1, 0);
	glVertexArrayAttribBinding(VAOImGui, 2, 0);

	glBindVertexArray(VAOImGui);

	// compile the shaders and link them into a shader program
	GLuint shaderVertexImGui = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shaderVertexImGui, 1, &shaderCodeVertexImGui, nullptr);
	glCompileShader(shaderVertexImGui);

	GLuint shaderFragmentImGui = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shaderFragmentImGui, 1, &shaderCodeFragmentImGui, nullptr);
	glCompileShader(shaderFragmentImGui);

	GLuint shaderProgramImGui = glCreateProgram();
	glAttachShader(shaderProgramImGui, shaderVertexImGui);
	glAttachShader(shaderProgramImGui, shaderFragmentImGui);
	glLinkProgram(shaderProgramImGui);
	glUseProgram(shaderProgramImGui);

	// create per frame uniform buffer for ImGui
	GLuint perFrameDataBufferImGui;
	glCreateBuffers(1, &perFrameDataBufferImGui);
	glNamedBufferStorage(perFrameDataBufferImGui, sizeof(mat4), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, perFrameDataBufferImGui);

	// ImGui graphics requires the following global opengl state
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);

	while (!glfwWindowShouldClose(app.getWindow()))
	{
		int width, height;
		glfwGetFramebufferSize(app.getWindow(), &width, &height);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		// start imgui rendering
		ImGuiIO& io    = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();
		ImGui::ShowDemoWindow();
		ImGui::Render();

		// construct a ortho proj for rendering Imgui 
		const ImDrawData* drawData        = ImGui::GetDrawData();
		const float       L               = drawData->DisplayPos.x;
		const float       R               = drawData->DisplayPos.x + drawData->DisplaySize.x;
		const float       T               = drawData->DisplayPos.y;
		const float       B               = drawData->DisplayPos.y + drawData->DisplaySize.y;
		const mat4        orthoProjection = glm::ortho(L, R, B, T);
		glNamedBufferSubData(perFrameDataBufferImGui, 0, sizeof(mat4), glm::value_ptr(orthoProjection));

		// execute commands from Imgui's command list 
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = drawData->CmdLists[n];
			glNamedBufferSubData(VBOImGui, 0, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), cmd_list->VtxBuffer.Data);
			glNamedBufferSubData(EBOImGui, 0, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), cmd_list->IdxBuffer.Data);

			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				const ImVec4     cr   = pcmd->ClipRect;
				glScissor((int)cr.x, (int)(height - cr.w), (int)(cr.z - cr.x), (int)(cr.w - cr.y));
				glBindTextureUnit(0, (GLuint)(intptr_t)pcmd->TextureId);
				glDrawElementsBaseVertex(GL_TRIANGLES,
				                         (GLsizei)pcmd->ElemCount,
				                         GL_UNSIGNED_SHORT,
				                         (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)),
				                         (GLint)pcmd->VtxOffset);
			}
		}

		// restore OpenGL state
		glScissor(0, 0, width, height);

		app.swapBuffers();
	}

	// clean up imgui
	ImGui::DestroyContext();

	// clean up the opengl objects
	glDeleteBuffers(1, &perFrameDataBufferImGui);
	glDeleteProgram(shaderProgramImGui);
	glDeleteShader(shaderVertexImGui);
	glDeleteShader(shaderFragmentImGui);
	glDeleteVertexArrays(1, &VAOImGui);

	return 0;
}
