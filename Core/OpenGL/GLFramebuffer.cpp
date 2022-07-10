#include "GLFramebuffer.h"
#include <cassert>


GLFramebuffer::GLFramebuffer(int width, int height, GLenum formatColor, GLenum formatDepth)
	: mWidth(width)
	, mHeight(height)
{
	glCreateFramebuffers(1, &mHandle);

	// if a texture format is set to 0, no corresponding buffer is created 

	if (formatColor)
	{
		mTexColor = std::make_unique<GLTexture>(GL_TEXTURE_2D, width, height, formatColor);

		// notice that we use GL_CLAMP_TO_EDGE to ensure proper filtering of color buffer
		glTextureParameteri(mTexColor->getHandle(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(mTexColor->getHandle(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glNamedFramebufferTexture(mHandle, GL_COLOR_ATTACHMENT0, mTexColor->getHandle(), 0);
	}
	if (formatDepth)
	{
		mTexDepth = std::make_unique<GLTexture>(GL_TEXTURE_2D, width, height, formatDepth);

		// notice that we use GL_CLAMP_TO_BORDER with 0 color to ensure proper filtering of depth buffer
		const GLfloat border[] = {0.0f, 0.0f, 0.0f, 0.0f};
		glTextureParameterfv(mTexDepth->getHandle(), GL_TEXTURE_BORDER_COLOR, border);
		glTextureParameteri(mTexDepth->getHandle(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTextureParameteri(mTexDepth->getHandle(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glNamedFramebufferTexture(mHandle, GL_DEPTH_ATTACHMENT, mTexDepth->getHandle(), 0);
	}

	const GLenum status = glCheckNamedFramebufferStatus(mHandle, GL_FRAMEBUFFER);

	assert(status == GL_FRAMEBUFFER_COMPLETE);
}

// notice that we use GL_FRAMEBUFFER when we use glBindFramebuffer.
// which makes both read and write framebuffers be set to the same framebuffer object
// However, it might be useful in situations where you want to make
// reading commands (glReadPixels()) and rendering commands using different framebuffer objects. 

GLFramebuffer::~GLFramebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &mHandle);
}

void GLFramebuffer::bind()
{
	// bind + set viewport. 
	glBindFramebuffer(GL_FRAMEBUFFER, mHandle);
	glViewport(0, 0, mWidth, mHeight);
}

void GLFramebuffer::unbind()
{
	// revert to default frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// TODO: after unbinding, it may be very handy to restore the viewport parameters
}
