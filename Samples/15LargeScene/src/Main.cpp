#include <glm/glm.hpp>

#include "OpenGL/GLApp.h"
#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLMesh.h"
#include "OpenGL/GLProgram.h"
#include "OpenGL/GLSceneData.h"
#include "OpenGL/GLShader.h"
#include "Util/Camera.h"

using glm::mat4;
using glm::vec4;
using glm::vec3;

const GLuint kBufferIndex_PerFrameUniforms = 0;

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

struct MouseState
{
	glm::vec2 pos         = glm::vec2(0.0f);
	bool      pressedLeft = false;
}             gMouseState;

CameraPositionerFirstPerson gPositioner(vec3(-10.0f, 3.0f, 3.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
Camera                      gCamera(gPositioner);

int main()
{
	GLApp app;

	GLShader  shdGridVertex("data/shaders/11DebugGrid/grid.vert");
	GLShader  shdGridFragment("data/shaders/11DebugGrid/grid.frag");
	GLProgram progGrid(shdGridVertex, shdGridFragment);

	const GLsizeiptr perFrameDataBufferSize = sizeof(PerFrameData);
	GLBuffer         perFrameDataBuffer(perFrameDataBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, kBufferIndex_PerFrameUniforms, perFrameDataBuffer.getHandle(), 0, perFrameDataBufferSize);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	GLShader  shaderVertex("data/shaders/15LargeScene/largeScene.vert");
	GLShader  shaderFragment("data/shaders/15LargeScene/largeScene.frag");
	GLProgram program(shaderVertex, shaderFragment);

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
		if (key == GLFW_KEY_SPACE)
			gPositioner.setUpVector(vec3(0.0f, 1.0f, 0.0f));
	});

	glfwSetCursorPosCallback(app.getWindow(), [](auto* window, double x, double y)
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		gMouseState.pos.x = static_cast<float>(x / width);
		gMouseState.pos.y = static_cast<float>(y / height);
	});

	glfwSetMouseButtonCallback(app.getWindow(), [](auto* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			gMouseState.pressedLeft = action == GLFW_PRESS;
	});

	gPositioner.mMaxSpeed = 1.0f;

	while (!glfwWindowShouldClose(app.getWindow()))
	{
		gPositioner.update(app.getDeltaSeconds(), gMouseState.pos, gMouseState.pressedLeft);

		int width, height;
		glfwGetFramebufferSize(app.getWindow(), &width, &height);
		const float ratio = width / (float)height;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const mat4 p    = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
		const mat4 view = gCamera.getViewMatrix();

		const PerFrameData perFrameData = {
			.view = view,
			.proj = p,
			.cameraPos = glm::vec4(gCamera.getPosition(), 1.0f)
		};
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, perFrameDataBufferSize, &perFrameData);

		glDisable(GL_BLEND);
		program.useProgram();
		mesh1.draw(sceneData1);
		mesh2.draw(sceneData2);

		glEnable(GL_BLEND);
		progGrid.useProgram();
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);

		app.swapBuffers();
	}

	return 0;
}
