#include "GLImGui.h"

GLImGui::GLImGui()
{
	glCreateVertexArrays(1, &mVAO);

	glVertexArrayElementBuffer(mVAO, mElementBuffer.getHandle());
	glVertexArrayVertexBuffer(mVAO, 0, mVertexBuffer.getHandle(), 0, sizeof(ImDrawVert));

	glEnableVertexArrayAttrib(mVAO, 0);
	glEnableVertexArrayAttrib(mVAO, 1);
	glEnableVertexArrayAttrib(mVAO, 2);

	glVertexArrayAttribFormat(mVAO, 0, 2, GL_FLOAT, GL_FALSE, IM_OFFSETOF(ImDrawVert, pos));
	glVertexArrayAttribFormat(mVAO, 1, 2, GL_FLOAT, GL_FALSE, IM_OFFSETOF(ImDrawVert, uv));
	glVertexArrayAttribFormat(mVAO, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, IM_OFFSETOF(ImDrawVert, col));

	glVertexArrayAttribBinding(mVAO, 0, 0);
	glVertexArrayAttribBinding(mVAO, 1, 0);
	glVertexArrayAttribBinding(mVAO, 2, 0);

	// Notice that we set the uniform to slot 7 to avoid collision with uniforms in the application
	glBindBufferBase(GL_UNIFORM_BUFFER, 7, mPerFrameDataBuffer.getHandle());

	defaultInitImGui();
}

GLImGui::~GLImGui()
{
	glDeleteVertexArrays(1, &mVAO);
	glDeleteTextures(1, &mTexture);
}

void GLImGui::render(int width, int height, const ImDrawData* draw_data)
{
	if (!draw_data)
		return;

	glBindTextures(0, 1, &mTexture);
	glBindVertexArray(mVAO);
	mProgram.useProgram();

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);

	const float     L               = draw_data->DisplayPos.x;
	const float     R               = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	const float     T               = draw_data->DisplayPos.y;
	const float     B               = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
	const glm::mat4 orthoProjection = glm::ortho(L, R, B, T);

	glNamedBufferSubData(mPerFrameDataBuffer.getHandle(), 0, sizeof(glm::mat4), glm::value_ptr(orthoProjection));

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];

		glNamedBufferSubData(mVertexBuffer.getHandle(),
		                     0,
		                     (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
		                     cmd_list->VtxBuffer.Data);

		glNamedBufferSubData(mElementBuffer.getHandle(),
		                     0,
		                     (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
		                     cmd_list->IdxBuffer.Data);

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

	glScissor(0, 0, width, height);
	glDisable(GL_SCISSOR_TEST);
}

void GLImGui::defaultInitImGui()
{
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	// Build texture atlas
	ImFontConfig cfg         = ImFontConfig();
	cfg.FontDataOwnedByAtlas = false;
	cfg.RasterizerMultiply   = 1.5f;
	cfg.SizePixels           = 1000.0f / 32.0f; // TODO: may need to change this if text doesn't look right
	cfg.PixelSnapH           = true;
	cfg.OversampleH          = 4;
	cfg.OversampleV          = 4;
	ImFont* Font             = io.Fonts->AddFontFromFileTTF("data/Inconsolata-Light.ttf", cfg.SizePixels, &cfg);

	unsigned char* pixels = nullptr;
	int            width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	glCreateTextures(GL_TEXTURE_2D, 1, &mTexture);
	glTextureParameteri(mTexture, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureParameteri(mTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(mTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureStorage2D(mTexture, 1, GL_RGBA8, width, height);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTextureSubImage2D(mTexture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	io.Fonts->TexID            = (ImTextureID)(intptr_t)mTexture;
	io.FontDefault             = Font;
	io.DisplayFramebufferScale = ImVec2(1, 1);
}
