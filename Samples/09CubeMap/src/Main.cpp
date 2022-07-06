#define GLFW_INCLUDE_NONE // with this line, glad and glfw3 could be included in any order
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLShader.h"
#include "OpenGL/GLProgram.h"
#include "OpenGL/GLTexture.h"

#include "Util/Debug.h"

using glm::mat4;
using glm::vec3;
using glm::vec4;
using glm::vec2;

#include <iostream>
using std::cout;
using std::endl;

#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

struct PerFrameData
{
	mat4 model;
	mat4 mvp;
	vec4 cameraPos;
};

struct VertexData
{
	vec3 pos;
	vec3 n;
	vec2 tc;
};

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
		// press escape key will close the window
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

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

	GLTexture duckBaseColor(GL_TEXTURE_2D, "data/rubber_duck/textures/Duck_baseColor.png");
	GLuint    duckBaseColorHandle = duckBaseColor.getHandle();
	glBindTextures(0, 1, &duckBaseColorHandle);

	GLTexture cubeMap(GL_TEXTURE_CUBE_MAP, "data/piazza_bologni_1k.hdr");
	GLuint    cubeMapHandle = cubeMap.getHandle();
	glBindTextures(1, 1, &cubeMapHandle);

	// depth test is required to render 3D objects
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	while (!glfwWindowShouldClose(window))
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		const float ratio = width / (float)height;
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

		{
			const mat4 m = glm::rotate(glm::translate(mat4(1.0f), vec3(0.0f, -0.5f, -1.5f)),
			                           (float)glfwGetTime(),
			                           vec3(0.0f, 1.0f, 0.0f));
			const PerFrameData perFrameData = {.model = m, .mvp = p * m, .cameraPos = vec4(0.0f)};
			glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, perFrameDataSize, &perFrameData);
			progModel.useProgram();
			glDrawArrays(GL_TRIANGLES, 0, static_cast<unsigned>(indices.size()));
		}

		{
			const mat4 m = glm::scale(mat4(1.0f), vec3(2.0f));

			const PerFrameData perFrameData = {.model = m, .mvp = p * m, .cameraPos = vec4(0.0f)};
			glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, perFrameDataSize, &perFrameData);
			progCube.useProgram();
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// clean up the opengl objects
	glDeleteVertexArrays(1, &vao);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
