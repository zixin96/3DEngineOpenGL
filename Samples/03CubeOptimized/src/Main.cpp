#define GLFW_INCLUDE_NONE // with this line, glad and glfw3 could be included in any order
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "OpenGL/GLApp.h"
#include "OpenGL/GLBuffer.h"
#include "OpenGL/GLProgram.h"
#include "OpenGL/GLShader.h"
using glm::mat4;
using glm::vec3;
using glm::vec4;

#include <iostream>
using std::cout;
using std::endl;

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

// this location value should match the one in the fragment shader
layout (location = 0) out vec3 color;

// procedural cube generation: 
const vec3 posData[8] = vec3[8](
  vec3(-1.0,-1.0, 1.0), vec3( 1.0,-1.0, 1.0),
  vec3(1.0, 1.0, 1.0), vec3(-1.0, 1.0, 1.0),
  vec3(-1.0,-1.0,-1.0), vec3(1.0,-1.0,-1.0),
  vec3( 1.0, 1.0,-1.0), vec3(-1.0, 1.0,-1.0)
);

const vec3 colorData[8] = vec3[8](
  vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0),
  vec3(0.0, 0.0, 1.0), vec3(1.0, 1.0, 0.0),
  vec3(1.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0),
  vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0)
);

const int indicesData[36] = int[36](
	// front
	0, 1, 2, 2, 3, 0,
	// right
	1, 5, 6, 6, 2, 1,
	// back
	7, 6, 5, 5, 4, 7,
	// left
	4, 0, 3, 3, 7, 4,
	// bottom
	4, 5, 1, 1, 0, 4,
	// top
	3, 2, 6, 6, 7, 3
);

void main()
{
	int idx = indicesData[gl_VertexID];
	gl_Position = mvp * vec4(posData[idx], 1.0);
	// if we are rendering a wire frame pass, set the vertex color to black
	color = isWireFrame > 0 ? vec3(0.0) : colorData[idx];
}

	)";

static const char* shaderCodeFragment = R"(
	#version 460 core

	// this location value should match the one in the vertex shader
	layout (location = 0) in vec3 color;

	layout (location = 0) out vec4 outFragColor;

	void main()
	{
		outFragColor = vec4(color, 1.0);
	}

	)";

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
	});

	// OpenGL uniform buffer offset alignment query
	GLint alignment;
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);

	// create an empty VAO
	GLuint VAO;
	glCreateVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// create per frame uniform buffer with no initial data
	GLsizeiptr perFrameDataSize = sizeof(PerFrameData);
	GLuint     elementCount     = 2;
	//!? make the buffer wtice as large and store 2 different copies of PerFrameData
	GLBuffer perFrameDataBuffer(elementCount * perFrameDataSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

	GLShader  shaderVertex(GL_VERTEX_SHADER, shaderCodeVertex, nullptr);
	GLShader  shaderFragment(GL_FRAGMENT_SHADER, shaderCodeFragment, nullptr);
	GLProgram shaderProgram(shaderVertex, shaderFragment);
	shaderProgram.useProgram();

	// depth test is required to render 3D objects
	glEnable(GL_DEPTH_TEST);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// polygon offset is needed to render a wire frame image of the cube on top of the solid image without Z-fighting
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(-1.f, -1.f); // move the wireframe rendering slightly toward the camera

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
			},
		};
		//!? update the entire buffer once
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, elementCount * perFrameDataSize, perFrameData);

		//!? feed the correct instance of PerFrameData into the shader

		glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle(), 0 * perFrameDataSize, perFrameDataSize);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle(), 1 * perFrameDataSize, perFrameDataSize);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		app.swapBuffers();
	}

	// clean up the opengl objects
	glDeleteVertexArrays(1, &VAO);

	return 0;
}
