#pragma once

#define GLFW_INCLUDE_NONE 
#include "GLFW/glfw3.h"

class GLApp
{
public:
	GLApp();

	~GLApp();

	GLFWwindow* getWindow() const { return mWindow; }
	float       getDeltaSeconds() const { return mDeltaSeconds; }

	void swapBuffers();

private:
	GLFWwindow* mWindow       = nullptr;
	double      mTimeStamp    = glfwGetTime();
	float       mDeltaSeconds = 0.f;
};
