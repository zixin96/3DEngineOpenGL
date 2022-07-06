#pragma once

#include <glad/gl.h>

class GLShader;

class GLProgram
{
public:
	GLProgram(const GLShader& a);
	GLProgram(const GLShader& a, const GLShader& b);
	GLProgram(const GLShader& a, const GLShader& b, const GLShader& c);
	GLProgram(const GLShader& a, const GLShader& b, const GLShader& c, const GLShader& d, const GLShader& e);
	~GLProgram();

	void   useProgram() const;
	GLuint getHandle() const { return mHandle; }

private:
	GLuint mHandle;
};