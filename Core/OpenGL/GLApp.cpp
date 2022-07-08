#include "GLApp.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <glad/gl.h>

#include "Util/Debug.h"

GLApp::GLApp()
{
	glfwSetErrorCallback([](int error, const char* description)
	{
		fprintf(stderr, "Error: %s\n", description);
	});

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

	int windowWidth, windowHeight;
	int monitorX, monitorY;

	const GLFWvidmode* info = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwGetMonitorPos(glfwGetPrimaryMonitor(), &monitorX, &monitorY);

	windowWidth = (int)(info->width * 0.85f);
	windowHeight = (int)(info->height * 0.85f);

	mWindow = glfwCreateWindow(windowWidth, windowHeight, "Simple example", nullptr, nullptr);
	glfwSetWindowPos(mWindow,
	                 monitorX + (info->width - windowWidth) / 2,
	                 monitorY + (info->height - windowHeight) / 2);

	if (!mWindow)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(mWindow);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(0);

	initDebug();
}

GLApp::~GLApp()
{
	glfwDestroyWindow(mWindow);
	glfwTerminate();
}

void GLApp::swapBuffers()
{
	glfwSwapBuffers(mWindow);
	glfwPollEvents();
	assert(glGetError() == GL_NO_ERROR);

	const double newTimeStamp = glfwGetTime();
	mDeltaSeconds             = static_cast<float>(newTimeStamp - mTimeStamp);
	mTimeStamp                = newTimeStamp;
}
