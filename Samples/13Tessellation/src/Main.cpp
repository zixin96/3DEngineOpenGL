#include <glm/glm.hpp>

#include "OpenGL/GLApp.h"
#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLMesh.h"
#include "OpenGL/GLProgram.h"
#include "OpenGL/GLShader.h"
#include "Util/Camera.h"
#include "Util/VtxData.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include "OpenGL/GLImGui.h"
#include "OpenGL/GLTexture.h"

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

struct PerFrameData
{
	mat4  view;
	mat4  proj;
	vec4  cameraPos;
	float tessellationScale;
};

struct VertexData
{
	vec3 pos;
	vec2 tc;
};

struct MouseState
{
	glm::vec2 pos         = glm::vec2(0.0f);
	bool      pressedLeft = false;
}             gMouseState;

CameraPositionerFirstPerson gPositioner(vec3(0.0f, 0.5f, 0.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
Camera                      gCamera(gPositioner);

float gTessellationScale = 1.0;

int main()
{
	GLApp app;

	GLShader  shdGridVertex("data/shaders/13Tessellation/grid.vert");
	GLShader  shdGridFragment("data/shaders/13Tessellation/grid.frag");
	GLProgram progGrid(shdGridVertex, shdGridFragment);

	GLShader  shaderVertex("data/shaders/13Tessellation/duck.vert");
	GLShader  shaderTessControl("data/shaders/13Tessellation/duck.tesc");
	GLShader  shaderTessEval("data/shaders/13Tessellation/duck.tese");
	GLShader  shaderGeometry("data/shaders/13Tessellation/duck.geom");
	GLShader  shaderFragment("data/shaders/13Tessellation/duck.frag");
	GLProgram program(shaderVertex, shaderTessControl, shaderTessEval, shaderGeometry, shaderFragment);


	const aiScene* scene = aiImportFile("data/rubber_duck/scene.gltf", aiProcess_Triangulate);

	if (!scene || !scene->HasMeshes())
	{
		printf("Unable to load data/rubber_duck/scene.gltf\n");
		exit(255);
	}

	const aiMesh*           mesh = scene->mMeshes[0];
	std::vector<VertexData> vertices;
	for (unsigned i = 0; i != mesh->mNumVertices; i++)
	{
		const aiVector3D v = mesh->mVertices[i];
		const aiVector3D t = mesh->mTextureCoords[0][i];
		vertices.push_back({.pos = vec3(v.x, v.z, v.y), .tc = vec2(t.x, t.y)});
	}
	std::vector<unsigned int> indices;
	for (unsigned i = 0; i != mesh->mNumFaces; i++)
	{
		for (unsigned j = 0; j != 3; j++)
			indices.push_back(mesh->mFaces[i].mIndices[j]);
	}
	aiReleaseImport(scene);

	const size_t     kSizeIndices           = sizeof(unsigned int) * indices.size();
	const size_t     kSizeVertices          = sizeof(VertexData) * vertices.size();
	const GLsizeiptr perFrameDataBufferSize = sizeof(PerFrameData);

	GLuint vao;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// indices
	GLBuffer dataIndices(kSizeIndices, indices.data(), 0);
	glVertexArrayElementBuffer(vao, dataIndices.getHandle());

	// vertices
	GLBuffer dataVertices(kSizeVertices, vertices.data(), 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, dataVertices.getHandle());

	// model matrices
	GLBuffer modelMatrices(sizeof(mat4), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, modelMatrices.getHandle());

	// per frame uniform buffer
	GLBuffer perFrameDataBuffer(perFrameDataBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle(), 0, perFrameDataBufferSize);

	GLTexture duckBaseColor(GL_TEXTURE_2D, "data/rubber_duck/textures/Duck_baseColor.png");
	GLuint    duckBaseColorHandle = duckBaseColor.getHandle();
	glBindTextures(0, 1, &duckBaseColorHandle);

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

		ImGui::GetIO().MousePos = ImVec2((float)x, (float)y);
	});

	glfwSetMouseButtonCallback(app.getWindow(), [](auto* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			gMouseState.pressedLeft = action == GLFW_PRESS;

		auto&     io      = ImGui::GetIO();
		const int idx     = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
		io.MouseDown[idx] = action == GLFW_PRESS;
	});

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	gPositioner.mMaxSpeed = 5.0f;

	GLImGui rendererUI;

	while (!glfwWindowShouldClose(app.getWindow()))
	{
		ImGuiIO& io = ImGui::GetIO();

		gPositioner.update(app.getDeltaSeconds(),
		                   gMouseState.pos,
		                   gMouseState.pressedLeft && !io.WantCaptureMouse); // "&& !io.WantCaptureMouse" part prevents camera from moving when we interact with imgui

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
			.cameraPos = glm::vec4(gCamera.getPosition(), 1.0f),
			.tessellationScale = gTessellationScale
		};
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, perFrameDataBufferSize, &perFrameData);

		const mat4 s = glm::scale(mat4(1.0f), vec3(10.0f));
		const mat4 m = s * glm::rotate(glm::translate(mat4(1.0f), vec3(0.0f, -0.5f, -1.5f)), (float)glfwGetTime() * 0.1f, vec3(0.0f, 1.0f, 0.0f));
		glNamedBufferSubData(modelMatrices.getHandle(), 0, sizeof(mat4), value_ptr(m));

		glBindVertexArray(vao);
		glEnable(GL_DEPTH_TEST);

		glDisable(GL_BLEND);
		program.useProgram();
		glBindTextures(0, 1, &duckBaseColorHandle);
		glDrawElements(GL_PATCHES, static_cast<unsigned>(indices.size()), GL_UNSIGNED_INT, nullptr);

		glEnable(GL_BLEND);
		progGrid.useProgram();
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);

		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();
		ImGui::SliderFloat("Tessellation scale", &gTessellationScale, 1.0f, 2.0f, "%.1f");
		ImGui::Render();
		rendererUI.render(width, height, ImGui::GetDrawData());

		app.swapBuffers();
	}

	return 0;
}
