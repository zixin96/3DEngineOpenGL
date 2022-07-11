#include <glm/glm.hpp>

#include "OpenGL/GLApp.h"
#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLCanvas.h"
#include "OpenGL/GLFramebuffer.h"
#include "OpenGL/GLImGui.h"
#include "OpenGL/GLMesh.h"
#include "OpenGL/GLMeshPVP.h"
#include "OpenGL/GLProgram.h"
#include "OpenGL/GLSceneData.h"
#include "OpenGL/GLShader.h"
#include "Util/Camera.h"

using glm::mat4;
using glm::vec4;
using glm::vec3;

const GLuint kBufferIndex_PerFrameUniforms = 0;
// kBufferIndex_Vertices is set to 0 in GLMeshPVP.cpp
const GLuint kBufferIndex_ModelMatrices = 1;

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

// the SSAO paraemters are chosen arbitrarily and can be tweaked using IMGUI
struct SSAOParams
{
	float scale     = 1.0f;
	float bias      = 0.2f;
	float zNear     = 0.1f;
	float zFar      = 1000.0f;
	float radius    = 0.2f;
	float attScale  = 1.0f;
	float distScale = 0.5f;
}         gSSAOParams;

// we reuse a single uniform buffer for both per-frame data and SSAO parameters,
// so make sure PerFrameData is larger enough for both structs
static_assert(sizeof(SSAOParams) <= sizeof(PerFrameData));

struct MouseState
{
	glm::vec2 pos         = glm::vec2(0.0f);
	bool      pressedLeft = false;
}             gMouseState;

CameraPositionerFirstPerson gPositioner(vec3(-10.0f, 3.0f, 3.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
Camera                      gCamera(gPositioner);

bool gEnableSSAO = true;
bool gEnableBlur = true;

int main()
{
	GLApp app;

	// shader program that renders the grid
	GLShader  shdGridVertex("data/shaders/11DebugGrid/grid.vert");
	GLShader  shdGridFragment("data/shaders/11DebugGrid/grid.frag");
	GLProgram progGrid(shdGridVertex, shdGridFragment);

	// shader program that renders the scene
	GLShader  shaderVertex("data/shaders/15LargeScene/largeScene.vert");
	GLShader  shaderFragment("data/shaders/15LargeScene/largeScene.frag");
	GLProgram program(shaderVertex, shaderFragment);

	// full screen vertex shader will be shared with multiple shader programs
	GLShader shdFullScreenQuadVert("data/shaders/fullScreenQuadOpt.vert");

	GLShader  shdSSAOFrag("data/shaders/16SSAO/SSAO.frag");
	GLProgram progSSAO(shdFullScreenQuadVert, shdSSAOFrag);

	GLShader  shdCombineSSAOFrag("data/shaders/16SSAO/SSAO_combine.frag");
	GLProgram progCombineSSAO(shdFullScreenQuadVert, shdCombineSSAOFrag);

	GLShader  shdBlurXFrag("data/shaders/16SSAO/BlurX.frag");
	GLProgram progBlurX(shdFullScreenQuadVert, shdBlurXFrag);

	GLShader  shdBlurYFrag("data/shaders/16SSAO/BlurY.frag");
	GLProgram progBlurY(shdFullScreenQuadVert, shdBlurYFrag);

	const GLsizeiptr perFrameDataBufferSize = sizeof(PerFrameData);
	GLBuffer         perFrameDataBuffer(perFrameDataBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, kBufferIndex_PerFrameUniforms, perFrameDataBuffer.getHandle(), 0, perFrameDataBufferSize);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	GLTexture rotationPattern(GL_TEXTURE_2D, "data/rot_texture.bmp");

	GLSceneData sceneData1("data/meshes/bistro_exterior.meshes", "data/meshes/bistro_exterior.scene", "data/meshes/bistro_exterior.materials");
	GLSceneData sceneData2("data/meshes/bistro_interior.meshes", "data/meshes/bistro_interior.scene", "data/meshes/bistro_interior.materials");

	GLMesh mesh1(sceneData1);
	GLMesh mesh2(sceneData2);

	// set a callback for key events
	glfwSetKeyCallback(app.getWindow(), [](GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		const bool pressed = action != GLFW_RELEASE;
		if (key == GLFW_KEY_ESCAPE && pressed)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		if (key == GLFW_KEY_W)
			gPositioner.mMovement.forward = pressed;
		if (key == GLFW_KEY_S)
			gPositioner.mMovement.backward = pressed;
		if (key == GLFW_KEY_A)
			gPositioner.mMovement.left = pressed;
		if (key == GLFW_KEY_D)
			gPositioner.mMovement.right = pressed;
		if (key == GLFW_KEY_1)
			gPositioner.mMovement.up = pressed;
		if (key == GLFW_KEY_2)
			gPositioner.mMovement.down = pressed;
		if (mods & GLFW_MOD_SHIFT)
			gPositioner.mMovement.fastSpeed = pressed;
		else
			gPositioner.mMovement.fastSpeed = false;
		if (key == GLFW_KEY_SPACE)
			gPositioner.setUpVector(vec3(0.0f, 1.0f, 0.0f));
	});

	glfwSetCursorPosCallback(app.getWindow(), [](auto* window, double x, double y)
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		gMouseState.pos.x       = static_cast<float>(x / width);
		gMouseState.pos.y       = static_cast<float>(y / height);
		ImGui::GetIO().MousePos = ImVec2((float)x, (float)y);
	});

	glfwSetMouseButtonCallback(app.getWindow(), [](auto* window, int button, int action, int mods)
	{
		auto&     io      = ImGui::GetIO();
		const int idx     = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
		io.MouseDown[idx] = action == GLFW_PRESS;

		if (!io.WantCaptureMouse)
			if (button == GLFW_MOUSE_BUTTON_LEFT)
				gMouseState.pressedLeft = action == GLFW_PRESS;
	});

	gPositioner.mMaxSpeed = 1.0f;

	// offscreen render targets
	int width, height;
	glfwGetFramebufferSize(app.getWindow(), &width, &height);
	GLFramebuffer framebuffer(width, height, GL_RGBA8, GL_DEPTH_COMPONENT24); // our scene will be rendered to this frame buffer

	// ssao and blur frame buffers will be used in a ping-pong fashion to do a multipass gaussian blur
	GLFramebuffer ssao(1024, 1024, GL_RGBA8, 0); // SSAO effect goes into a 1024x1024 buffer and no depth buffer
	GLFramebuffer blur(1024, 1024, GL_RGBA8, 0); // blur is the same as SSAO effects

	GLImGui rendererUI;

	while (!glfwWindowShouldClose(app.getWindow()))
	{
		gPositioner.update(app.getDeltaSeconds(), gMouseState.pos, gMouseState.pressedLeft);

		int width, height;
		glfwGetFramebufferSize(app.getWindow(), &width, &height);
		const float ratio = width / (float)height;

		// clear the frame buffer 
		glClearNamedFramebufferfv(framebuffer.getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
		glClearNamedFramebufferfi(framebuffer.getHandle(), GL_DEPTH_STENCIL, 0, 1.0f, 0);

		// update view and projection matrix
		const mat4         p            = glm::perspective(45.0f, ratio, gSSAOParams.zNear, gSSAOParams.zFar);
		const mat4         view         = gCamera.getViewMatrix();
		const PerFrameData perFrameData = {.view = view, .proj = p, .cameraPos = glm::vec4(gCamera.getPosition(), 1.0f)};
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, perFrameDataBufferSize, &perFrameData);

		// 1. Render scene
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		framebuffer.bind();
		// 1.1 Bistro
		program.useProgram();
		mesh1.draw(sceneData1);
		mesh2.draw(sceneData2);
		// 1.2 Grid
		glEnable(GL_BLEND);
		progGrid.useProgram();
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);
		framebuffer.unbind();
		glDisable(GL_DEPTH_TEST);

		// 2. Calculate SSAO
		glClearNamedFramebufferfv(ssao.getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, sizeof(gSSAOParams), &gSSAOParams);
		ssao.bind();
		progSSAO.useProgram();
		glBindTextureUnit(0, framebuffer.getTextureDepth().getHandle()); // pass the depth texture of the main framebuffer into the SSAO shader (location = 0)
		glBindTextureUnit(1, rotationPattern.getHandle());               // pass a special 2D texture with the rotation pattern into the SSAO shader (location = 1)
		glDrawArrays(GL_TRIANGLES, 0, 3);                                //!? Note: for optimized full screen, count == 3
		ssao.unbind();

		// 2.1 Blur SSAO
		if (gEnableBlur)
		{
			// Blur X
			blur.bind();
			progBlurX.useProgram();
			glBindTextureUnit(0, ssao.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 3);
			blur.unbind();
			// Blur Y
			ssao.bind();
			progBlurY.useProgram();
			glBindTextureUnit(0, blur.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 3);
			ssao.unbind();
		}

		// 3. Combine SSAO and the rendered scene
		glViewport(0, 0, width, height);
		if (gEnableSSAO)
		{
			progCombineSSAO.useProgram();
			glBindTextureUnit(0, framebuffer.getTextureColor().getHandle());
			glBindTextureUnit(1, ssao.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
		else
		{
			// if SSAO is disabled, copy offscreen framebuffer right into the app's main window
			glBlitNamedFramebuffer(framebuffer.getHandle(), 0, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}

		ImGuiIO& io    = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();

		ImGui::Begin("Control", nullptr);
		ImGui::Checkbox("Enable SSAO", &gEnableSSAO);
		ImGui::BeginDisabled(!gEnableSSAO);
		ImGui::GetStyle().DisabledAlpha = 0.2f;
		ImGui::Checkbox("Enable blur", &gEnableBlur);
		ImGui::SliderFloat("SSAO scale", &gSSAOParams.scale, 0.0f, 2.0f);
		ImGui::SliderFloat("SSAO bias", &gSSAOParams.bias, 0.0f, 0.3f);
		ImGui::EndDisabled();

		ImGui::Separator();

		ImGui::SliderFloat("SSAO radius", &gSSAOParams.radius, 0.05f, 0.5f);
		ImGui::SliderFloat("SSAO attenuation scale", &gSSAOParams.attScale, 0.5f, 1.5f);
		ImGui::SliderFloat("SSAO distance scale", &gSSAOParams.distScale, 0.0f, 1.0f);
		ImGui::End();

		imguiTextureWindowGL("Color", framebuffer.getTextureColor().getHandle());
		imguiTextureWindowGL("Depth", framebuffer.getTextureDepth().getHandle());
		imguiTextureWindowGL("SSAO", ssao.getTextureColor().getHandle());

		ImGui::Render();
		rendererUI.render(width, height, ImGui::GetDrawData());

		app.swapBuffers();
	}

	return 0;
}
