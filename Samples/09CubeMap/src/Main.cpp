#define GLFW_INCLUDE_NONE // with this line, glad and glfw3 could be included in any order
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLShader.h"
#include "OpenGL/GLProgram.h"
#include "OpenGL/GLTexture.h"
#include "Util/Camera.h"
#include "Util/Debug.h"

using glm::mat4;
using glm::vec3;
using glm::vec4;
using glm::vec2;

#include <iostream>
using std::cout;
using std::endl;

#include <vector>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

struct VertexData
{
	vec3 pos;
	vec3 n;
	vec2 tc;
};

struct MouseState
{
	glm::vec2 pos         = glm::vec2(0.0f);
	bool      pressedLeft = false;
}             gMouseState;

CameraPositionerFirstPerson gPositioner(vec3(0.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
Camera                      gCamera(gPositioner);

int main()
{
	// set the GLFW error callback via a simple lambda to catch potential errors
	glfwSetErrorCallback([](int error, const char* description)
	{
		fprintf(stderr, "Error: %s\n", description);
	});

	// now we can initialize GLFW
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	// tell GLFW which OpenGL version to use
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

	GLFWwindow* window = glfwCreateWindow(1000, 1000, "GLFW", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// set a callback for key events
	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
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

		// press F9 to save a screenshot 
		if (key == GLFW_KEY_F9 && action == GLFW_PRESS)
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			uint8_t* ptr = (uint8_t*)malloc(width * height * 4);
			glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
			stbi_write_png("images/screenshot.png", width, height, 4, ptr, 0);
			free(ptr);
		}
	});

	glfwSetCursorPosCallback(window, [](auto* window, double x, double y)
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		gMouseState.pos.x = static_cast<float>(x / width);
		gMouseState.pos.y = static_cast<float>(y / height);
	});

	glfwSetMouseButtonCallback(window, [](auto* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			gMouseState.pressedLeft = action == GLFW_PRESS;
	});

	// prepare OpenGL context
	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(1);

	// OpenGL Debugging
	initDebug();

	GLShader  shdModelVertex("data/shaders/09CubeMap/08Duck.vert");
	GLShader  shdModelFragment("data/shaders/09CubeMap/08Duck.frag");
	GLProgram progModel(shdModelVertex, shdModelFragment);

	GLShader  shdCubeVertex("data/shaders/09CubeMap/08Cube.vert");
	GLShader  shdCubeFragment("data/shaders/09CubeMap/08Cube.frag");
	GLProgram progCube(shdCubeVertex, shdCubeFragment);

	// create per frame uniform buffer with no initial data
	GLsizeiptr perFrameDataSize = sizeof(PerFrameData);
	GLBuffer   perFrameDataBuffer(perFrameDataSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle(), 0, perFrameDataSize);

	const aiScene* scene = aiImportFile("data/rubber_duck/scene.gltf", aiProcess_Triangulate);
	if (!scene || !scene->HasMeshes())
	{
		printf("Unable to load data/rubber_duck/scene.gltf\n");
		exit(255);
	}

	const aiMesh* mesh = scene->mMeshes[0];

	std::vector<VertexData> vertices;
	for (unsigned int i = 0; i != mesh->mNumVertices; i++)
	{
		const aiVector3D pos = mesh->mVertices[i];
		const aiVector3D n   = mesh->mNormals[i];
		const aiVector3D tc  = mesh->mTextureCoords[0][i];
		vertices.push_back({
			                   .pos = vec3(pos.x, pos.z, pos.y),
			                   .n = vec3(n.x, n.y, n.z),
			                   .tc = vec2(tc.x, tc.y)
		                   });
	}

	std::vector<unsigned int> indices;
	for (unsigned int i = 0; i != mesh->mNumFaces; i++)
	{
		for (unsigned int j = 0; j != 3; j++)
		{
			indices.push_back(mesh->mFaces[i].mIndices[j]);
		}
	}

	aiReleaseImport(scene);

	const GLsizeiptr vertexBufferByteSize = sizeof(VertexData) * vertices.size();
	const GLsizeiptr indexBufferByteSize  = sizeof(unsigned int) * indices.size();

	GLBuffer vertexBuffer(vertexBufferByteSize, vertices.data(), 0);
	GLBuffer indexBuffer(indexBufferByteSize, indices.data(), 0);

	GLuint vao;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexBuffer.getHandle());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, indexBuffer.getHandle());

	// model matrices
	GLBuffer modelMatrices(2 * sizeof(mat4), nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, modelMatrices.getHandle());

	GLTexture duckBaseColor(GL_TEXTURE_2D, "data/rubber_duck/textures/Duck_baseColor.png");
	GLuint    duckBaseColorHandle = duckBaseColor.getHandle();
	glBindTextures(0, 1, &duckBaseColorHandle);

	GLTexture cubeMap(GL_TEXTURE_CUBE_MAP, "data/piazza_bologni_1k.hdr");
	GLuint    cubeMapHandle = cubeMap.getHandle();
	glBindTextures(1, 1, &cubeMapHandle);

	// depth test is required to render 3D objects
	glEnable(GL_DEPTH_TEST);
	// for sky dome rendering, we need to adjust the depth func
	glDepthFunc(GL_LEQUAL);
	// since you are inside a cube, you need to consider disabling back face culling as well
	// in this we don't need to
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	double timeStamp    = glfwGetTime();
	float  deltaSeconds = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		gPositioner.update(deltaSeconds, gMouseState.pos, gMouseState.pressedLeft);

		const double newTimeStamp = glfwGetTime();
		deltaSeconds              = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp                 = newTimeStamp;

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		const float ratio = width / (float)height;
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// bind per-frame data in the scene once
		const mat4         projMat      = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
		const mat4         viewMat      = gCamera.getViewMatrix();
		const PerFrameData perFrameData = {
			.view = viewMat,
			.proj = projMat,
			.cameraPos = glm::vec4(gCamera.getPosition(), 1.0f)
		};
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, perFrameDataSize, &perFrameData);

		// bind all model matrices in the scene once
		const mat4 Matrices[2]
		{
			glm::rotate(glm::translate(mat4(1.0f), vec3(0.0f, -0.5f, -1.5f)), (float)glfwGetTime(), vec3(0.0f, 1.0f, 0.0f)),
			glm::scale(mat4(1.0f), vec3(10.0f))
		};
		glNamedBufferSubData(modelMatrices.getHandle(), 0, sizeof(mat4) * 2, Matrices);

		progModel.useProgram();
		// baseInstance = 0 => gl_BaseInstance = 0 => use gl_BaseInstance to fetch the right model matrix
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, static_cast<unsigned>(indices.size()), 1, 0);

		progCube.useProgram();
		// baseInstance = 1 => gl_BaseInstance = 1 => use gl_BaseInstance to fetch the right model matrix
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 36, 1, 1);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// clean up the opengl objects
	glDeleteVertexArrays(1, &vao);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
