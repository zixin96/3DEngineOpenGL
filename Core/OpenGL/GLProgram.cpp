#include "GLProgram.h"
#include "GLShader.h"

#include <cassert>
#include <cstdio>

static void printProgramInfoLog(GLuint handle)
{
	char    buffer[8192];
	GLsizei length = 0;
	glGetProgramInfoLog(handle, sizeof(buffer), &length, buffer);
	if (length)
	{
		printf("%s\n", buffer);
		assert(false);
	}
}

GLProgram::GLProgram(const GLShader& a)
	: mHandle(glCreateProgram())
{
	glAttachShader(mHandle, a.getHandle());
	glLinkProgram(mHandle);
	printProgramInfoLog(mHandle);
}

GLProgram::GLProgram(const GLShader& a, const GLShader& b)
	: mHandle(glCreateProgram())
{
	glAttachShader(mHandle, a.getHandle());
	glAttachShader(mHandle, b.getHandle());
	glLinkProgram(mHandle);
	printProgramInfoLog(mHandle);
}

GLProgram::GLProgram(const GLShader& a, const GLShader& b, const GLShader& c)
	: mHandle(glCreateProgram())
{
	glAttachShader(mHandle, a.getHandle());
	glAttachShader(mHandle, b.getHandle());
	glAttachShader(mHandle, c.getHandle());
	glLinkProgram(mHandle);
	printProgramInfoLog(mHandle);
}

GLProgram::GLProgram(const GLShader& a, const GLShader& b, const GLShader& c, const GLShader& d, const GLShader& e)
	: mHandle(glCreateProgram())
{
	glAttachShader(mHandle, a.getHandle());
	glAttachShader(mHandle, b.getHandle());
	glAttachShader(mHandle, c.getHandle());
	glAttachShader(mHandle, d.getHandle());
	glAttachShader(mHandle, e.getHandle());
	glLinkProgram(mHandle);
	printProgramInfoLog(mHandle);
}

GLProgram::~GLProgram()
{
	glDeleteProgram(mHandle);
}

void GLProgram::useProgram() const
{
	glUseProgram(mHandle);
}
