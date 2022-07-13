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

const int SHADOW_MAP_SIZE = 1024;

float gLightAngle      = 60.0f; // FOV of the light 
float gLightInnerAngle = 10.0f;
float gLightNear       = 1.0f;
float gLightFar        = 20.0f;
float gLightDist       = 12.0f;

// the rotation angles around the local X and Y axis
float gLightXAngle = -1.0f;
float gLightYAngle = -2.0f;

bool gRotateModel = true;

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 lightViewProj; // assume we have a single light
	vec4 cameraPos;
	vec4 lightAngles; // stores cosines of the light's inner and outer angles: cos(inner), cos(outer)
	vec4 lightPos;
};

struct MouseState
{
	glm::vec2 pos         = glm::vec2(0.0f);
	bool      pressedLeft = false;
}             gMouseState;

CameraPositionerFirstPerson gPositioner(vec3(0.0f, 6.0f, 11.0f), vec3(0.0f, 4.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
Camera                      gCamera(gPositioner);

int main()
{
	GLApp app;

	// shader program that renders the grid
	GLShader  shdGridVertex("data/shaders/11DebugGrid/grid.vert");
	GLShader  shdGridFragment("data/shaders/11DebugGrid/grid.frag");
	GLProgram progGrid(shdGridVertex, shdGridFragment);

	// shader program that renders the scene
	GLShader  shdModelVert("data/shaders/16ShadowMapping/scene.vert");
	GLShader  shdModelFrag("data/shaders/16ShadowMapping/scene.frag");
	GLProgram progModel(shdModelVert, shdModelFrag);

	// shader program that renders the shadow
	GLShader  shdShadowVert("data/shaders/16ShadowMapping/shadow.vert");
	GLShader  shdShadowFrag("data/shaders/16ShadowMapping/shadow.frag");
	GLProgram progShadowMap(shdShadowVert, shdShadowFrag);

	// create meshes in the scene
	GLMeshPVP jet("data/rubber_duck/scene.gltf");
	//? note we have two base color textures, does it mean that we have two meshes in this model? 
	GLTexture texAlbedoJet(GL_TEXTURE_2D, "data/rubber_duck/textures/Duck_baseColor.png");

	const std::vector<uint32_t>   indices  = {0, 1, 2, 2, 3, 0};
	const std::vector<VertexData> vertices = {
		{vec3(-2, -2, 0), vec3(0, 0, 1), vec2(0, 0)},
		{vec3(-2, +2, 0), vec3(0, 0, 1), vec2(0, 1)},
		{vec3(+2, +2, 0), vec3(0, 0, 1), vec2(1, 1)},
		{vec3(+2, -2, 0), vec3(0, 0, 1), vec2(1, 0)},
	};
	GLMeshPVP plane(indices, vertices.data(), uint32_t(sizeof(VertexData) * vertices.size()));
	GLTexture texAlbedoPlane(GL_TEXTURE_2D, "data/ch2_sample3_STB.jpg");

	const std::vector<GLMeshPVP*> meshesToDraw = {&jet, &plane};

	// create shadow map (technically you won't need a color buffer, but we provide one for visualization purposes)
	GLFramebuffer shadowMap(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, GL_RGBA8, GL_DEPTH_COMPONENT24);

	// create canvas and imgui renderer
	GLImGui  rendererUI;
	GLCanvas canvas;

	// per-frame UBO
	const GLsizeiptr perFrameDataBufferSize = sizeof(PerFrameData);
	GLBuffer         perFrameDataBuffer(perFrameDataBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, kBufferIndex_PerFrameUniforms, perFrameDataBuffer.getHandle(), 0, perFrameDataBufferSize);

	// model matrices
	const mat4 m(1.0f);
	GLBuffer   modelMatrices(sizeof(mat4), value_ptr(m), GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_ModelMatrices, modelMatrices.getHandle());

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

	gPositioner.mMaxSpeed = 5.0f;
	float angle           = 0.0f;

	while (!glfwWindowShouldClose(app.getWindow()))
	{
		gPositioner.update(app.getDeltaSeconds(), gMouseState.pos, gMouseState.pressedLeft);

		int width, height;
		glfwGetFramebufferSize(app.getWindow(), &width, &height);
		const float ratio = width / (float)height;

		if (gRotateModel)
		{
			angle += app.getDeltaSeconds();
		}

		// Render shadow map
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		// Calculate light parameters (which we can control via IMGUI)
		const mat4 rotY      = rotate(mat4(1.f), gLightYAngle, vec3(0, 1, 0));
		const mat4 rotX      = rotate(rotY, gLightXAngle, vec3(1, 0, 0));
		const vec4 lightPos  = rotX * vec4(0, 0, gLightDist, 1.0f);
		const mat4 lightProj = glm::perspective(glm::radians(gLightAngle), 1.0f, gLightNear, gLightFar);
		const mat4 lightView = lookAt(vec3(lightPos), vec3(0), vec3(0, 1, 0));

		{
			const PerFrameData perFrameData = {
				// notice that some fields are uninitialized b/c we won't use them in the shadow pass
				.view = lightView,
				.proj = lightProj,
				.cameraPos = vec4(gCamera.getPosition(), 1.0f)
			};
			glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, perFrameDataBufferSize, &perFrameData);
			shadowMap.bind();
			// before rendering, clear the color and depth buffers of the shadow-map frame buffer
			glClearNamedFramebufferfv(shadowMap.getHandle(), GL_COLOR, 0, value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
			glClearNamedFramebufferfi(shadowMap.getHandle(), GL_DEPTH_STENCIL, 0, 1.0f, 0);
			progShadowMap.useProgram();
			for (const auto& m : meshesToDraw)
				m->drawElements();
			shadowMap.unbind();
		}

		// Render scene
		glViewport(0, 0, width, height); // restore OpenGL viewport
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		const mat4         proj         = glm::perspective(45.0f, ratio, 0.5f, 5000.0f);
		const mat4         view         = gCamera.getViewMatrix();
		const PerFrameData perFrameData = {
			.view = view,
			.proj = proj,
			.lightViewProj = lightProj * lightView,
			.cameraPos = vec4(gCamera.getPosition(), 1.0f),
			.lightAngles = vec4(cosf(glm::radians(0.5f * gLightAngle)),
			                    cosf(glm::radians(0.5f * (gLightAngle - gLightInnerAngle))),
			                    1.0f,
			                    1.0f),
			.lightPos = lightPos
		};
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, perFrameDataBufferSize, &perFrameData);

		// Position the mesh+plane to where we want
		const mat4 scale = glm::scale(mat4(1.0f), vec3(3.0f));
		const mat4 rot   = rotate(mat4(1.0f), glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
		const mat4 pos   = translate(mat4(1.0f), vec3(0.0f, 0.0f, +1.0f));
		const mat4 m     = rotate(scale * rot * pos, angle, vec3(0.0f, 0.0f, 1.0f));
		glNamedBufferSubData(modelMatrices.getHandle(), 0, sizeof(mat4), value_ptr(m));

		const GLuint textures[] = {texAlbedoJet.getHandle(), texAlbedoPlane.getHandle()};
		glBindTextureUnit(1, shadowMap.getTextureDepth().getHandle()); // shadow map always at location = 1
		progModel.useProgram();
		for (size_t i = 0; i != meshesToDraw.size(); i++)
		{
			glBindTextureUnit(0, textures[i]); // different meshes have different albedo textures (at location = 0)
			meshesToDraw[i]->drawElements();
		}

		glEnable(GL_BLEND);
		progGrid.useProgram();
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);

		// render a debug light frustum
		renderCameraFrustumGL(canvas, lightView, lightProj, vec4(0.0f, 1.0f, 0.0f, 1.0f));
		canvas.flush();

		// render IMGUI
		ImGuiIO& io    = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();

		ImGui::Begin("Control", nullptr);
		ImGui::Checkbox("Rotate model", &gRotateModel);
		ImGui::Text("Light parameters", nullptr);
		ImGui::SliderFloat("Proj::Light angle", &gLightAngle, 15.0f, 170.0f);
		ImGui::SliderFloat("Proj::Light inner angle", &gLightInnerAngle, 1.0f, 15.0f);
		ImGui::SliderFloat("Proj::Near", &gLightNear, 0.1f, 5.0f);
		ImGui::SliderFloat("Proj::Far", &gLightFar, 0.1f, 100.0f);
		ImGui::SliderFloat("Pos::Dist", &gLightDist, 0.5f, 100.0f);
		ImGui::SliderFloat("Pos::AngleX", &gLightXAngle, -3.15f, +3.15f);
		ImGui::SliderFloat("Pos::AngleY", &gLightYAngle, -3.15f, +3.15f);
		ImGui::End();

		imguiTextureWindowGL("Color", shadowMap.getTextureColor().getHandle());
		imguiTextureWindowGL("Depth", shadowMap.getTextureDepth().getHandle());

		ImGui::Render();
		rendererUI.render(width, height, ImGui::GetDrawData());

		app.swapBuffers();
	}

	return 0;
}
