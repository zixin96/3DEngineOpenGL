#pragma once

#include <glad/gl.h>

class GLShader
{
public:
	explicit GLShader(const char* fileName);
	GLShader(GLenum type, const char* text, const char* debugFileName = "");
	~GLShader();
	GLenum getType() const { return mType; }
	GLuint getHandle() const { return mHandle; }

private:
	GLenum mType;
	GLuint mHandle;
}; 