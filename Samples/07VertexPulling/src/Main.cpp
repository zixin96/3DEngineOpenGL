#define GLFW_INCLUDE_NONE // with this line, glad and glfw3 could be included in any order
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "OpenGL/GLApp.h"
#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLShader.h"
#include "OpenGL/GLProgram.h"
#include "OpenGL/GLTexture.h"

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
	mat4 mvp;
};

struct VertexData
{
	vec3 pos;
	vec2 tc;
};

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

	GLShader  shaderVertex("data/shaders/07VertexPulling/07.vert");
	GLShader  shaderGeometry("data/shaders/07VertexPulling/07.geom");
	GLShader  shaderFragment("data/shaders/07VertexPulling/07.frag");
	GLProgram program(shaderVertex, shaderGeometry, shaderFragment);
	program.useProgram();

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
		const aiVector3D tc  = mesh->mTextureCoords[0][i];
		vertices.push_back({
			                   .pos = vec3(pos.x, pos.z, pos.y),
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
	glVertexArrayElementBuffer(vao, indexBuffer.getHandle());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexBuffer.getHandle());

	GLTexture duckBaseColor(GL_TEXTURE_2D, "data/rubber_duck/textures/Duck_baseColor.png");
	GLuint    duckBaseColorHandle = duckBaseColor.getHandle();
	glBindTextures(0, 1, &duckBaseColorHandle);

	// depth test is required to render 3D objects
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	while (!glfwWindowShouldClose(app.getWindow()))
	{
		int width, height;
		glfwGetFramebufferSize(app.getWindow(), &width, &height);
		const float ratio = width / (float)height;
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const mat4 m = glm::rotate(glm::translate(mat4(1.0f), vec3(0.0f, -0.5f, -1.5f)), (float)glfwGetTime(), vec3(0.0f, 1.0f, 0.0f));
		const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

		// render the solid cube with FILL mode
		PerFrameData perFrameData[1] = {
			{
				.mvp = p * m
			}
		};
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, perFrameDataSize, perFrameData);
		glDrawElements(GL_TRIANGLES, static_cast<unsigned>(indices.size()), GL_UNSIGNED_INT, nullptr);

		app.swapBuffers();
	}

	// clean up the opengl objects
	glDeleteVertexArrays(1, &vao);

	return 0;
}
