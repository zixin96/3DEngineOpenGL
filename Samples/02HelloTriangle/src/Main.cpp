#define GLFW_INCLUDE_NONE // with this line, glad and glfw3 could be included in any order
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <iostream>

#include "OpenGL/GLProgram.h"
#include "OpenGL/GLShader.h"
using std::cout;
using std::endl;

//  Let's draw a colored triangle
// For the sake of brevity, all error checking is omitted 

// We use the GLSL built-in gl_VertexID input variable to index into the pos[]
// and col[] arrays to generate the vertex positions and colors programmatically
// In this case, no user-defined inputs to the vertex shader are required.
static const char* shaderCodeVertex = R"(
	#version 460 core

	// this location value should match the one in the fragment shader
	layout (location = 0) out vec3 color;

	const vec2 posData[3] = vec2[3](
		vec2(-0.6, -0.4),
		vec2(0.6, -0.4),
		vec2(0.0, 0.6)
	);

	const vec3 colorData[3] = vec3[3](
		vec3(1.0, 0.0, 0.0),
		vec3(0.0, 1.0, 0.0),
		vec3(0.0, 0.0, 1.0)
	);

	void main()
	{
		gl_Position = vec4(posData[gl_VertexID], 0.0, 1.0);
		color = colorData[gl_VertexID];
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
	// we will use OpenGL 4.6 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
	});

	// prepare OpenGL context
	glfwMakeContextCurrent(window);
	// use the GLAD library to import all OpenGL entry points and extensions
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(1);

	// create a VAO
	// For this example, we will use the vertex shader to generate
	// all vertex data, so an empty VAO will be sufficient
	GLuint VAO;
	glCreateVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	GLShader  shaderVertex(GL_VERTEX_SHADER, shaderCodeVertex, nullptr);
	GLShader  shaderFragment(GL_FRAGMENT_SHADER, shaderCodeFragment, nullptr);
	GLProgram shaderProgram(shaderVertex, shaderFragment);
	shaderProgram.useProgram();

	// The main loop starts by checking whether the window should be closed
	while (!glfwWindowShouldClose(window))
	{
		// Implement a resizable window by reading the current width and height from GLFW
		// and updating the OpenGL viewport accordingly
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		// Clear the screen
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// render the triangle
		// The glDrawArrays() function can be invoked with the empty VAO that we bound earlier
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// The fragment shader output was rendered into the back buffer. Let's swap the front
		// and back buffers to make the triangle visible
		glfwSwapBuffers(window);

		// poll the events
		glfwPollEvents();
	}

	// clean up the opengl objects
	glDeleteVertexArrays(1, &VAO);

	// terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
