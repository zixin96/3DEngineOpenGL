#pragma once

#include <glad/gl.h>
#include "GLTexture.h"
#include <memory>

class GLFramebuffer
{
public:
	GLFramebuffer(int width, int height, GLenum formatColor, GLenum formatDepth);
	~GLFramebuffer();
	GLFramebuffer(const GLFramebuffer&) = delete;
	GLFramebuffer(GLFramebuffer&&)      = default;
	GLuint           getHandle() const { return mHandle; }
	const GLTexture& getTextureColor() const { return *mTexColor; }
	const GLTexture& getTextureDepth() const { return *mTexDepth; }
	void             bind();
	void             unbind();

private:
	int    mWidth;
	int    mHeight;
	GLuint mHandle = 0;

	// TODO: multiple render targets (MRT) may need more than just one of each buffer
	std::unique_ptr<GLTexture> mTexColor;
	std::unique_ptr<GLTexture> mTexDepth;
};
