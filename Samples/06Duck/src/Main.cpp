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
	int  isWireFrame;
	//!? pad our structure to 256 bytes to satisfy uniform buffer offset alignment for this machine
	int  p1[3];
	vec4 p2[11];
};

static const char* shaderCodeVertex = R"(
	#version 460 core

	// this binding value should match what we bind the uniform buffer
	// notice that the uniform buffer matches the C++ structure
	layout (std140, binding = 0) uniform PerFrameData
	{
		mat4 mvp;
		int isWireFrame;
		//!? match paddings with C++ struct
		int p1[3];
		vec4 p2[11];
	};

	layout (location = 0) in vec3 pos;
	layout (location = 0) out vec3 color;

	void main()
	{
		gl_Position = mvp * vec4(pos, 1.0);
		color = isWireFrame > 0 ? vec3(0.0f) : pos.xyz;
	}

	)";

static const char* shaderCodeFragment = R"(
	#version 460 core
	layout (location = 0) out vec4 outFragColor;
	layout (location = 0) in vec3 color;

	void main()
	{
		outFragColor = vec4(color, 1.0);
	}

	)";

std::vector<vec3> LoadModel()
{
	const aiScene* scene = aiImportFile("data/rubber_duck/scene.gltf", aiProcess_Triangulate);
	if (!scene || !scene->HasMeshes())
	{
		printf("Unable to load data/rubber_duck/scene.gltf\n");
		exit(255);
	}

	const aiMesh* mesh = scene->mMeshes[0];

	std::vector<vec3> positions;
	for (unsigned int i = 0; i != mesh->mNumFaces; i++)
	{
		const aiFace&      face   = mesh->mFaces[i];
		const unsigned int idx[3] = {face.mIndices[0], face.mIndices[1], face.mIndices[2]};
		for (int j = 0; j != 3; j++)
		{
			const aiVector3D v = mesh->mVertices[idx[j]];
			positions.push_back(vec3(v.x, v.z, v.y));
		}
	}

	aiReleaseImport(scene);
	return positions;
}

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

	std::vector<vec3> positions   = LoadModel();
	const int         numVertices = static_cast<int>(positions.size());

	GLuint VAO;
	GLuint meshData;
	glCreateVertexArrays(1, &VAO);
	glCreateBuffers(1, &meshData);
	glNamedBufferStorage(meshData, sizeof(vec3) * positions.size(), positions.data(), 0);
	glVertexArrayVertexBuffer(VAO, 0, meshData, 0, sizeof(vec3));
	glEnableVertexArrayAttrib(VAO, 0);
	glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(VAO, 0, 0);

	glBindVertexArray(VAO);

	// create per frame uniform buffer with no initial data
	GLsizeiptr perFrameDataSize = sizeof(PerFrameData);
	GLuint     elementCount     = 2;
	GLuint     perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	glNamedBufferStorage(perFrameDataBuffer, elementCount * perFrameDataSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

	// compile the shaders and link them into a shader program
	const GLuint shaderVertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shaderVertex, 1, &shaderCodeVertex, nullptr);
	glCompileShader(shaderVertex);

	const GLuint shaderFragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shaderFragment, 1, &shaderCodeFragment, nullptr);
	glCompileShader(shaderFragment);

	const GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, shaderVertex);
	glAttachShader(shaderProgram, shaderFragment);
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// depth test is required to render 3D objects
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_LINE); // polygon offset is needed to render a wire frame image of the cube on top of the solid image without Z-fighting
	glPolygonOffset(-1.f, -1.f);      // move the wireframe rendering slightly toward the camera

	while (!glfwWindowShouldClose(app.getWindow()))
	{
		int width, height;
		glfwGetFramebufferSize(app.getWindow(), &width, &height);
		const float ratio = width / (float)height;
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const mat4 m = glm::rotate(glm::translate(mat4(1.0f), vec3(0.0f, 0.0f, -3.5f)),
		                           (float)glfwGetTime(),
		                           vec3(1.f, 1.f, .1f));
		const mat4 p = glm::perspective(45.f, ratio, 0.1f, 1000.f);

		// render the solid cube with FILL mode
		PerFrameData perFrameData[2] = {
			{
				.mvp = p * m,
				.isWireFrame = false
			},
			{
				.mvp = p * m,
				.isWireFrame = true
			}
		};
		//!? update the entire buffer once
		glNamedBufferSubData(perFrameDataBuffer, 0, elementCount * perFrameDataSize, perFrameData);

		//!? feed the correct instance of PerFrameData into the shader

		glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0 * perFrameDataSize, perFrameDataSize);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawArrays(GL_TRIANGLES, 0, numVertices);

		glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 1 * perFrameDataSize, perFrameDataSize);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, numVertices);

		app.swapBuffers();
	}

	// clean up the opengl objects
	glDeleteBuffers(1, &perFrameDataBuffer);
	glDeleteProgram(shaderProgram);
	glDeleteShader(shaderVertex);
	glDeleteShader(shaderFragment);
	glDeleteVertexArrays(1, &VAO);

	return 0;
}
