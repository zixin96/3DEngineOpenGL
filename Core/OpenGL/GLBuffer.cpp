#include "GLBuffer.h"

GLBuffer::GLBuffer(GLsizeiptr size, const void* data, GLbitfield flags)
{
	glCreateBuffers(1, &mHandle);
	// Allocates size bytes of OpenGL server memory for storing data 
	glNamedBufferStorage(
	                     // the buffer affected
	                     mHandle,
	                     // number of bytes to allocate
	                     size,
	                     // initial data
	                     data,
	                     // tells the OpenGL implementation that how we will use the content of the data 
	                     flags);
}

GLBuffer::~GLBuffer()
{
	glDeleteBuffers(1, &mHandle);
}
