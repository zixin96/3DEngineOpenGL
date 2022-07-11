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

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

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
layout (location = 0) out vec2 uv;

const vec3 vertices[36] = vec3[36](
	// positions        
	vec3(-1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f,  1.0f, -1.0f),
	vec3( 1.0f,  1.0f, -1.0f),
	vec3(-1.0f,  1.0f, -1.0f),
	vec3(-1.0f, -1.0f, -1.0f),

	vec3(-1.0f, -1.0f,  1.0f),
	vec3( 1.0f, -1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3(-1.0f,  1.0f,  1.0f),
	vec3(-1.0f, -1.0f,  1.0f),

	vec3(-1.0f,  1.0f,  1.0f),
	vec3(-1.0f,  1.0f, -1.0f),
	vec3(-1.0f, -1.0f, -1.0f),
	vec3(-1.0f, -1.0f, -1.0f),
	vec3(-1.0f, -1.0f,  1.0f),
	vec3(-1.0f,  1.0f,  1.0f),

	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	
	vec3(-1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f, -1.0f),
	vec3( 1.0f, -1.0f,  1.0f),
	vec3( 1.0f, -1.0f,  1.0f),
	vec3(-1.0f, -1.0f,  1.0f),
	vec3(-1.0f, -1.0f, -1.0f),

	vec3(-1.0f,  1.0f, -1.0f),
	vec3( 1.0f,  1.0f, -1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3( 1.0f,  1.0f,  1.0f),
	vec3(-1.0f,  1.0f,  1.0f),
	vec3(-1.0f,  1.0f, -1.0f)
);

const vec2 uvs[36] = vec2[36](
	// texture coord
	vec2(0.0f,  0.0f),
	vec2(1.0f,  0.0f),
	vec2(1.0f,  1.0f),
	vec2(1.0f,  1.0f),
	vec2(0.0f,  1.0f),
	vec2(0.0f,  0.0f),

	vec2(0.0f,  0.0f),
	vec2(1.0f,  0.0f),
	vec2(1.0f,  1.0f),
	vec2(1.0f,  1.0f),
	vec2(0.0f,  1.0f),
	vec2(0.0f,  0.0f),

	vec2(1.0f,  0.0f),
	vec2(1.0f,  1.0f),
	vec2(0.0f,  1.0f),
	vec2(0.0f,  1.0f),
	vec2(0.0f,  0.0f),
	vec2(1.0f,  0.0f),

	vec2(1.0f,  0.0f),
	vec2(1.0f,  1.0f),
	vec2(0.0f,  1.0f),
	vec2(0.0f,  1.0f),
	vec2(0.0f,  0.0f),
	vec2(1.0f,  0.0f),

	vec2(0.0f,  1.0f),
	vec2(1.0f,  1.0f),
	vec2(1.0f,  0.0f),
	vec2(1.0f,  0.0f),
	vec2(0.0f,  0.0f),
	vec2(0.0f,  1.0f),

	vec2(0.0f,  1.0f),
	vec2(1.0f,  1.0f),
	vec2(1.0f,  0.0f),
	vec2(1.0f,  0.0f),
	vec2(0.0f,  0.0f),
	vec2(0.0f,  1.0f)
);

void main()
{
	gl_Position = mvp * vec4(vertices[gl_VertexID], 1.0);
	// if we are rendering a wire frame pass, set the vertex color to black
	uv = uvs[gl_VertexID];
}

	)";

static const char* shaderCodeFragment = R"(
	#version 460 core

	// this location value should match the one in the vertex shader
	layout (location = 0) in vec2 uv;

	layout (location = 0) out vec4 outFragColor;

	layout (binding = 0) uniform sampler2D texture0;

	void main()
	{
		outFragColor = texture(texture0, uv);
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
	GLuint     perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	//!? make the buffer wtice as large and store 2 different copies of PerFrameData
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

	int            w, h, comp;
	const uint8_t* img = stbi_load("data/snowWood.jpg", &w, &h, &comp, 3);
	GLuint         texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureStorage2D(texture, 1, GL_RGB8, w, h);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTextureSubImage2D(texture, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img);
	glBindTextures(0, 1, &texture);

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
		glNamedBufferSubData(perFrameDataBuffer, 0, elementCount * perFrameDataSize, perFrameData);

		//!? feed the correct instance of PerFrameData into the shader

		glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0 * perFrameDataSize, perFrameDataSize);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 1 * perFrameDataSize, perFrameDataSize);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, 36);

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
