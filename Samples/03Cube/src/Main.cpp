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

#include <iostream>
using std::cout;
using std::endl;

// declare a C++ structure to hold our uniform buffer data
struct PerFrameData
{
	// store the premultiplied model-view-projection matrix
	mat4 mvp;
	//  used to set the color of the wireframe rendering
	int isWireframe;
};

static const char* shaderCodeVertex = R"(
#version 460 core

// this binding value should match what we bind the uniform buffer
// notice that the uniform buffer matches the C++ structure
layout (std140, binding = 0) uniform PerFrameData
{
	uniform mat4 mvp;
	uniform int isWireFrame;
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

	// create an empty VAO
	GLuint VAO;
	glCreateVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// create per frame uniform buffer with no initial data
	const GLsizeiptr perFrameDataBufferSize = sizeof(PerFrameData);
	GLBuffer         perFrameDataBuffer(perFrameDataBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	// Make the entire buffer accessible from GLSL shaders at binding point 0
	glBindBufferRange(
	                  // the buffer binding targets
	                  GL_UNIFORM_BUFFER,
	                  // the index associated with a uniform block
	                  // This value should be used in the shader code to read data from the buffer
	                  0,
	                  // the buffer object
	                  perFrameDataBuffer.getHandle(),
	                  // the following two specify the starting offset and range of the buffer that is to be mapped to the uniform buffer
	                  0,
	                  perFrameDataBufferSize);


	GLShader  shaderVertex(GL_VERTEX_SHADER, shaderCodeVertex, nullptr);
	GLShader  shaderFragment(GL_FRAGMENT_SHADER, shaderCodeFragment, nullptr);
	GLProgram shaderProgram(shaderVertex, shaderFragment);
	shaderProgram.useProgram();

	// depth test is required to render 3D objects
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// polygon offset is needed to render a wire frame image of the cube on top of the solid image without Z-fighting
	// OpenGL Redbook: Page 171
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(-1.f, -1.f); // move the wireframe rendering slightly toward the camera

	while (!glfwWindowShouldClose(app.getWindow()))
	{
		int width, height;
		glfwGetFramebufferSize(app.getWindow(), &width, &height);
		const float ratio = width / (float)height;
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// To rotate the cube, the model matrix is calculated:
		const mat4 m = glm::rotate(
		                           glm::translate(mat4(1.0f), vec3(0.0f, 0.0f, -3.5f)),
		                           //  the angle of rotation is based on the current system time returned by glfwGetTime()
		                           (float)glfwGetTime(),
		                           // To rotate the cube, the model matrix is calculated as a rotation around the diagonal (1, 1, 1) axis
		                           vec3(1.0f, 1.0f, 1.0f));

		const mat4 p = glm::perspective(45.f, ratio, 0.1f, 1000.f);

		// To highlight the edges of our object, we first draw the object with polygon mode set to GL_FILL
		// and then draw it again in a different color and with the polygon mode set to GL_LINE

		PerFrameData perFrameData = {
			.mvp = p * m,
			.isWireframe = false
		};
		// Replaces a subset of a buffer object’s data store with new data
		glNamedBufferSubData(
		                     // buffer to be updated
		                     perFrameDataBuffer.getHandle(),
		                     // The section of the buffer object specified in buffer starting at offset bytes is updated with
		                     // the size bytes of data addressed by data:
		                     // offset
		                     0,
		                     // size
		                     perFrameDataBufferSize,
		                     // data
		                     &perFrameData);
		// render the solid cube with the polygon mode set to GL_FILL
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		// Note: gl_VertexID ranges from [0, 36), which is specified here
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// update the buffer for the second draw call
		perFrameData.isWireframe = true;
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, perFrameDataBufferSize, &perFrameData);
		//  render the wireframe cube using the GL_LINE polygon mode
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		app.swapBuffers();
	}

	// clean up the opengl objects
	glDeleteVertexArrays(1, &VAO);

	return 0;
}
