#include "GLBuffer.h"

GLBuffer::GLBuffer(GLsizeiptr size, const void* data, GLbitfield flags)
{
	glCreateBuffers(1, &mHandle);
	glNamedBufferStorage(mHandle, size, data, flags);
}

GLBuffer::~GLBuffer()
{
	glDeleteBuffers(1, &mHandle);
}
