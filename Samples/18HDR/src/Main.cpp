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
const GLuint kBufferIndex_ModelMatrices = 1;

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

struct HDRParams
{
	float exposure      = 0.9f;
	float maxWhite      = 1.17f;
	float bloomStrength = 1.1f;
}         gHDRParams;


// we reuse a single uniform buffer for both per-frame data and HDR parameters,
// so make sure PerFrameData is larger enough for both structs
static_assert(sizeof(HDRParams) <= sizeof(PerFrameData));

struct MouseState
{
	glm::vec2 pos         = glm::vec2(0.0f);
	bool      pressedLeft = false;
}             gMouseState;

CameraPositionerFirstPerson gPositioner(vec3(-15.81f, 5.18f, -5.81f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
Camera                      gCamera(gPositioner);

bool gEnableHDR = true;

int main()
{
	GLApp app;

	// shader program that renders the grid
	GLShader  shdGridVertex("data/shaders/11DebugGrid/grid.vert");
	GLShader  shdGridFragment("data/shaders/11DebugGrid/grid.frag");
	GLProgram progGrid(shdGridVertex, shdGridFragment);

	// shader program that renders the scene with IBL
	GLShader  shaderVertex("data/shaders/18HDR/scene_IBL.vert");
	GLShader  shaderFragment("data/shaders/18HDR/scene_IBL.frag");
	GLProgram program(shaderVertex, shaderFragment);

	// full screen vertex shader will be shared with multiple shader programs
	GLShader shdFullScreenQuadVert("data/shaders/fullScreenQuadOpt.vert");

	GLShader  shdCombineHDR("data/shaders/18HDR/HDR.frag");
	GLProgram progCombineHDR(shdFullScreenQuadVert, shdCombineHDR);

	GLShader  shdToLuminance("data/shaders/18HDR/ToLuminance.frag");
	GLProgram progToLuminance(shdFullScreenQuadVert, shdToLuminance);

	GLShader  shdBrightPass("data/shaders/18HDR/BrightPass.frag");
	GLProgram progBrightPass(shdFullScreenQuadVert, shdBrightPass);

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
	GLFramebuffer framebuffer(width, height, GL_RGBA16F, GL_DEPTH_COMPONENT24);
	GLFramebuffer luminance(64, 64, GL_R16F, 0);
	GLFramebuffer brightPass(256, 256, GL_RGBA16F, 0);
	GLFramebuffer bloom1(256, 256, GL_RGBA16F, 0);
	GLFramebuffer bloom2(256, 256, GL_RGBA16F, 0);
	// create a texture view into the last mip-level (1x1 pixel) of our luminance framebuffer
	GLuint luminance1x1;
	glGenTextures(1, &luminance1x1);
	glTextureView(luminance1x1, GL_TEXTURE_2D, luminance.getTextureColor().getHandle(), GL_R16F, 6, 1, 0, 1);
	const GLint Mask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
	glTextureParameteriv(luminance1x1, GL_TEXTURE_SWIZZLE_RGBA, Mask);

	// cube map
	// https://hdrihaven.com/hdri/?h=immenstadter_horn
	GLTexture envMap(GL_TEXTURE_CUBE_MAP, "data/immenstadter_horn_2k.hdr");
	GLTexture envMapIrradiance(GL_TEXTURE_CUBE_MAP, "data/immenstadter_horn_2k_irradiance.hdr");
	GLShader  shdCubeVertex("data/shaders/chapter08/GL03_cube.vert");
	GLShader  shdCubeFragment("data/shaders/chapter08/GL03_cube.frag");
	GLProgram progCube(shdCubeVertex, shdCubeFragment);
	GLuint    dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);
	const GLuint pbrTextures[] = {envMap.getHandle(), envMapIrradiance.getHandle()};
	glBindTextures(5, 2, pbrTextures);

	GLImGui rendererUI;

	while (!glfwWindowShouldClose(app.getWindow()))
	{
		gPositioner.update(app.getDeltaSeconds(), gMouseState.pos, gMouseState.pressedLeft);

		int width, height;
		glfwGetFramebufferSize(app.getWindow(), &width, &height);
		const float ratio = width / (float)height;

		//

		app.swapBuffers();
	}

	return 0;
}
