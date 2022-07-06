#pragma once

#include <glad/gl.h>

class GLTexture
{
public:
	GLTexture(GLenum type, const char* fileName);
	GLTexture(GLenum type, const char* fileName, GLenum clamp);
	GLTexture(GLenum type, int width, int height, GLenum internalFormat);
	GLTexture(int w, int h, const void* img);
	~GLTexture();
	GLTexture(const GLTexture&) = delete;
	GLTexture(GLTexture&&);
	GLenum   getType() const { return mType; }
	GLuint   getHandle() const { return mHandle; }
	GLuint64 getHandleBindless() const { return mHandleBindless; }

private:
	GLenum   mType           = 0;
	GLuint   mHandle         = 0;
	GLuint64 mHandleBindless = 0;
};
