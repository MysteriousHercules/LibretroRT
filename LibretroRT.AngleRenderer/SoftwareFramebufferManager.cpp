#include "pch.h"
#include "SoftwareFramebufferManager.h"

#include <math.h>

using namespace LibretroRT;
using namespace LibretroRT_AngleRenderer;

const unsigned SoftwareFramebufferManager::TextureMinSize = 1024;

SoftwareFramebufferManager::SoftwareFramebufferManager():
	mTextureID(NULL),
	mTextureSize(0),
	mTexturePixelFormat(PixelFormats::FormatUknown)
{
}

SoftwareFramebufferManager::~SoftwareFramebufferManager()
{
	DeleteTexture();
}

GLuint SoftwareFramebufferManager::TextureId()
{
	return mTextureID;
}

void SoftwareFramebufferManager::UpdateTextureFormat(GameGeometry^ geometry, PixelFormats pixelFormat)
{
	auto requestedSize = max(max(geometry->MaxWidth, geometry->MaxHeight), TextureMinSize);
	if (pixelFormat == mTexturePixelFormat && requestedSize <= mTextureSize)
	{
		return;
	}

	DeleteTexture();
	glGenTextures(1, &mTextureID);
	glBindTexture(GL_TEXTURE_2D, mTextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	auto converterPixelFormat = LibretroToGLPixelFormat(pixelFormat);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, requestedSize, requestedSize, 0, converterPixelFormat, GL_UNSIGNED_BYTE, nullptr);
}

void SoftwareFramebufferManager::DeleteTexture()
{
	if (mTextureID != NULL)
	{
		glDeleteTextures(1, &mTextureID);
	}

	mTextureID = NULL;
}

GLint SoftwareFramebufferManager::LibretroToGLPixelFormat(PixelFormats pixelFormat)
{
	switch (pixelFormat)
	{
	case PixelFormats::FormatRGB565:
		return GL_RGB565;
	case PixelFormats::FormatXRGB8888:
		return GL_RGBA;
	default:
		return NULL;
	}
}