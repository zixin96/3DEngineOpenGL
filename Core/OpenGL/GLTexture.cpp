#include "GLTexture.h"
#include "Util/Bitmap.h"
#include "Util/UtilsCubemap.h"

#include <glad/gl.h>
#include <cassert>
#include <cstdio>
#include <string>

#include <stb_image_write.h>
#include <stb/stb_image.h>
//#include <gli/gli.hpp>
//#include <gli/texture2d.hpp>
//#include <gli/load_ktx.hpp>

static int getNumMipMapLevels2D(int w, int h)
{
	int levels = 1;
	while ((w | h) >> levels)
		levels += 1;
	return levels;
}

// Draw a checkerboard on a pre-allocated square RGB image.
static uint8_t* genDefaultCheckerboardImage(int* width, int* height)
{
	const int w = 128;
	const int h = 128;

	uint8_t* imgData = (uint8_t*)malloc(w * h * 3); // stbi_load() uses malloc(), so this is safe

	assert(imgData && w > 0 && h > 0);
	assert(w == h);

	if (!imgData || w <= 0 || h <= 0) return nullptr;
	if (w != h) return nullptr;

	for (int i = 0; i < w * h; i++)
	{
		const int row      = i / w;
		const int col      = i % w;
		imgData[i * 3 + 0] = imgData[i * 3 + 1] = imgData[i * 3 + 2] = 0xFF * ((row + col) % 2);
	}

	if (width) *width = w;
	if (height) *height = h;

	return imgData;
}

GLTexture::GLTexture(GLenum type, int width, int height, GLenum internalFormat)
	: mType(type)
{
	glCreateTextures(type, 1, &mHandle);
	glTextureParameteri(mHandle, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(mHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureStorage2D(mHandle, getNumMipMapLevels2D(width, height), internalFormat, width, height);
}


GLTexture::GLTexture(GLenum type, const char* fileName)
	: GLTexture(type, fileName, GL_REPEAT)
{
}

GLTexture::GLTexture(GLenum type, const char* fileName, GLenum clamp)
	: mType(type)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glCreateTextures(type, 1, &mHandle);
	glTextureParameteri(mHandle, GL_TEXTURE_MAX_LEVEL, 0);
	glTextureParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(mHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(mHandle, GL_TEXTURE_WRAP_S, clamp);
	glTextureParameteri(mHandle, GL_TEXTURE_WRAP_T, clamp);

	const char* ext = strrchr(fileName, '.');

	const bool isKTX = ext && !strcmp(ext, ".ktx");

	switch (type)
	{
		case GL_TEXTURE_2D:
			{
				int w          = 0;
				int h          = 0;
				int numMipmaps = 0;
				if (isKTX)
				{
					assert(false);
					// gli::texture          gliTex = gli::load_ktx(fileName);
					// gli::gl               GL(gli::gl::PROFILE_KTX);
					// gli::gl::format const format = GL.translate(gliTex.format(), gliTex.swizzles());
					// glm::tvec3<GLsizei>   extent(gliTex.extent(0));
					// w          = extent.x;
					// h          = extent.y;
					// numMipmaps = getNumMipMapLevels2D(w, h);
					// glTextureStorage2D(mHandle, numMipmaps, format.Internal, w, h);
					// glTextureSubImage2D(mHandle, 0, 0, 0, w, h, format.External, format.Type, gliTex.data(0, 0, 0));
				}
				else
				{
					uint8_t* img = stbi_load(fileName, &w, &h, nullptr, STBI_rgb_alpha);

					// Note(Anton): replaced assert(img) with a fallback image to prevent crashes with missing files or bad (eg very long) paths.
					if (!img)
					{
						fprintf(stderr, "WARNING: could not load image `%s`, using a fallback.\n", fileName);
						img = genDefaultCheckerboardImage(&w, &h);
						if (!img)
						{
							fprintf(stderr, "FATAL ERROR: out of memory allocating image for fallback texture\n");
							exit(EXIT_FAILURE);
						}
					}

					numMipmaps = getNumMipMapLevels2D(w, h);
					glTextureStorage2D(mHandle, numMipmaps, GL_RGBA8, w, h);
					glTextureSubImage2D(mHandle, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, img);
					stbi_image_free((void*)img);
				}
				glGenerateTextureMipmap(mHandle);
				glTextureParameteri(mHandle, GL_TEXTURE_MAX_LEVEL, numMipmaps - 1);
				glTextureParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTextureParameteri(mHandle, GL_TEXTURE_MAX_ANISOTROPY, 16);
				break;
			}
		case GL_TEXTURE_CUBE_MAP:
			{
				// assume that the cube map is either in equirectangular format or vertical cross format

				int w, h, comp;
				// use stb's floating-point API to load a HDR range cube map image from a .hdr file
				const float* img = stbi_loadf(fileName, &w, &h, &comp, 3);
				assert(img);
				Bitmap in(w, h, comp, eBitmapFormat::Float, img);
				stbi_image_free((void*)img);

				// is this cube map equirectangular? 
				const bool isEquirectangular = w == 2 * h;
				// if so, convert it to vertical cross format. o.w., it is a vertical cross format. 
				Bitmap out = isEquirectangular ? convertEquirectangularMapToVerticalCross(in) : in;

				#ifndef NDEBUG
				stbi_write_hdr("images/cubemap_verticalcross.hdr",
				               out.mWidth,
				               out.mHeight,
				               out.mComp,
				               (const float*)out.mData.data());
				#endif

				Bitmap cubemap = convertVerticalCrossToCubeMapFaces(out);

				const int numMipmaps = getNumMipMapLevels2D(cubemap.mWidth, cubemap.mHeight);

				glTextureParameteri(mHandle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTextureParameteri(mHandle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTextureParameteri(mHandle, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTextureParameteri(mHandle, GL_TEXTURE_BASE_LEVEL, 0);
				glTextureParameteri(mHandle, GL_TEXTURE_MAX_LEVEL, numMipmaps - 1);
				glTextureParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTextureParameteri(mHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
				glTextureStorage2D(mHandle, numMipmaps, GL_RGB32F, cubemap.mWidth, cubemap.mHeight);
				const uint8_t* data = cubemap.mData.data();

				for (unsigned i = 0; i != 6; ++i)
				{
					glTextureSubImage3D(mHandle, 0, 0, 0, i, cubemap.mWidth, cubemap.mHeight, 1, GL_RGB, GL_FLOAT, data);
					data += cubemap.mWidth * cubemap.mHeight * cubemap.mComp * Bitmap::getBytesPerComponent(cubemap.mFmt);
				}

				glGenerateTextureMipmap(mHandle);
				break;
			}
		default:
			assert(false);
	}

	mHandleBindless = glGetTextureHandleARB(mHandle);
	glMakeTextureHandleResidentARB(mHandleBindless);
}

GLTexture::GLTexture(int w, int h, const void* img)
	: mType(GL_TEXTURE_2D)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glCreateTextures(mType, 1, &mHandle);
	int numMipmaps = getNumMipMapLevels2D(w, h);
	glTextureStorage2D(mHandle, numMipmaps, GL_RGBA8, w, h);
	glTextureSubImage2D(mHandle, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, img);
	glGenerateTextureMipmap(mHandle);
	glTextureParameteri(mHandle, GL_TEXTURE_MAX_LEVEL, numMipmaps - 1);
	glTextureParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(mHandle, GL_TEXTURE_MAX_ANISOTROPY, 16);
	mHandleBindless = glGetTextureHandleARB(mHandle);
	glMakeTextureHandleResidentARB(mHandleBindless);
}

GLTexture::GLTexture(GLTexture&& other)
	: mType(other.mType)
	, mHandle(other.mHandle)
	, mHandleBindless(other.mHandleBindless)
{
	other.mType           = 0;
	other.mHandle         = 0;
	other.mHandleBindless = 0;
}

GLTexture::~GLTexture()
{
	if (mHandleBindless)
		glMakeTextureHandleNonResidentARB(mHandleBindless);
	glDeleteTextures(1, &mHandle);
}
