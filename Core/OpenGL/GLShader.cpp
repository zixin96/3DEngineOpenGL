#include "GLShader.h"
#include "Util/Utils.h"

#include <cassert>

static GLenum GLShaderTypeFromFileName(const char* fileName)
{
	if (endsWith(fileName, ".vert"))
		return GL_VERTEX_SHADER;

	if (endsWith(fileName, ".frag"))
		return GL_FRAGMENT_SHADER;

	if (endsWith(fileName, ".geom"))
		return GL_GEOMETRY_SHADER;

	if (endsWith(fileName, ".tesc"))
		return GL_TESS_CONTROL_SHADER;

	if (endsWith(fileName, ".tese"))
		return GL_TESS_EVALUATION_SHADER;

	if (endsWith(fileName, ".comp"))
		return GL_COMPUTE_SHADER;

	assert(false);

	return 0;
}


GLShader::GLShader(const char* fileName)
	: GLShader(GLShaderTypeFromFileName(fileName),
	           readShaderFile(fileName).c_str(), fileName)
{
}

GLShader::GLShader(GLenum type, const char* text, const char* debugFileName)
	: mType(type)
	, mHandle(glCreateShader(type))
{
	glShaderSource(mHandle, 1, &text, nullptr);
	glCompileShader(mHandle);

	char    buffer[8192];
	GLsizei length = 0;
	glGetShaderInfoLog(mHandle, sizeof(buffer), &length, buffer);

	if (length)
	{
		printf("%s (File: %s)\n", buffer, debugFileName);
		printShaderSource(text);
		assert(false);
	}
}

GLShader::~GLShader()
{
	glDeleteShader(mHandle);
}
