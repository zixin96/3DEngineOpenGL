#pragma once

#include <glad/gl.h>

class GLBuffer
{
public:
	GLBuffer(GLsizeiptr size, const void* data, GLbitfield flags);
	~GLBuffer();

	GLuint getHandle() const { return mHandle; }

private:
	GLuint mHandle;
};
