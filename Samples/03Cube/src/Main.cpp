#define GLFW_INCLUDE_NONE // with this line, glad and glfw3 could be included in any order
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
using glm::mat4;
using glm::vec3;

#include <iostream>
using std::cout;
using std::endl;

void APIENTRY glDebugOutput(GLenum       source,
                            GLenum       type,
                            unsigned int id,
                            GLenum       severity,
                            GLsizei      length,
                            const char*  message,
                            const void*  userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	cout << "---------------" << endl;
	cout << "Debug message (" << id << "): " << message << endl;

	switch (source)
	{
		case GL_DEBUG_SOURCE_API: cout << "Source: API";
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: cout << "Source: Window System";
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: cout << "Source: Shader Compiler";
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY: cout << "Source: Third Party";
			break;
		case GL_DEBUG_SOURCE_APPLICATION: cout << "Source: Application";
			break;
		case GL_DEBUG_SOURCE_OTHER: cout << "Source: Other";
			break;
	}
	cout << endl;

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR: cout << "Type: Error";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: cout << "Type: Deprecated Behaviour";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: cout << "Type: Undefined Behaviour";
			break;
		case GL_DEBUG_TYPE_PORTABILITY: cout << "Type: Portability";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE: cout << "Type: Performance";
			break;
		case GL_DEBUG_TYPE_MARKER: cout << "Type: Marker";
			break;
		case GL_DEBUG_TYPE_PUSH_GROUP: cout << "Type: Push Group";
			break;
		case GL_DEBUG_TYPE_POP_GROUP: cout << "Type: Pop Group";
			break;
		case GL_DEBUG_TYPE_OTHER: cout << "Type: Other";
			break;
	}
	cout << endl;

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH: cout << "Severity: high";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM: cout << "Severity: medium";
			break;
		case GL_DEBUG_SEVERITY_LOW: cout << "Severity: low";
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: cout << "Severity: notification";
			break;
	}
	cout << endl;
	cout << endl;
}

struct PerFrameData
{
	mat4 mvp;
	int  isWireFrame;
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
	});

	// prepare OpenGL context
	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(1);

	// OpenGL Debugging
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		// initialize debug output
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}

	// create an empty VAO
	GLuint VAO;
	glCreateVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// create per frame uniform buffer with no initial data
	const GLsizeiptr perFrameDataBufferSize = sizeof(PerFrameData);
	GLuint           perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	glNamedBufferStorage(perFrameDataBuffer, perFrameDataBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, perFrameDataBufferSize);

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

	// polygon offset is needed to render a wire frame image of the cube on top of the solid image without Z-fighting
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(-1.f, -1.f); // move the wireframe rendering slightly toward the camera

	while (!glfwWindowShouldClose(window))
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		const float ratio = width / (float)height;
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const mat4 m = glm::rotate(glm::translate(mat4(1.0f), vec3(0.0f, 0.0f, -3.5f)),
		                           (float)glfwGetTime(),
		                           vec3(1.f, 1.f, .1f));

		const mat4 p = glm::perspective(45.f, ratio, 0.1f, 1000.f);

		// render the solid cube with FILL mode
		PerFrameData perFrameData = {
			.mvp = p * m,
			.isWireFrame = false
		};
		glNamedBufferSubData(perFrameDataBuffer, 0, perFrameDataBufferSize, &perFrameData);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// render the solid cube with LINE mode
		perFrameData.isWireFrame = true;
		glNamedBufferSubData(perFrameDataBuffer, 0, perFrameDataBufferSize, &perFrameData);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// clean up the opengl objects
	glDeleteBuffers(1, &perFrameDataBuffer);
	glDeleteProgram(shaderProgram);
	glDeleteShader(shaderVertex);
	glDeleteShader(shaderFragment);
	glDeleteVertexArrays(1, &VAO);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
