#include "pch.h"
#include "SoftwareCoreRenderer.h"

#include <math.h>

using namespace LibretroRT;
using namespace LibretroRT_AngleRenderer;

const unsigned SoftwareCoreRenderer::TextureMinSize = 1024;

SoftwareCoreRenderer::SoftwareCoreRenderer():
	mFramebufferID(NULL),
	mFramebufferSize(0),
	mFramebufferPixelFormat(PixelFormats::FormatUknown)
{
}

SoftwareCoreRenderer::~SoftwareCoreRenderer()
{
	DeleteFramebuffer();
}

void SoftwareCoreRenderer::UpdateFramebufferFormat(GameGeometry^ geometry, PixelFormats pixelFormat)
{
	auto requestedSize = max(max(geometry->MaxWidth, geometry->MaxHeight), TextureMinSize);
	if (pixelFormat == mFramebufferPixelFormat && requestedSize <= mFramebufferSize)
	{
		return;
	}

	DeleteFramebuffer();
	glGenTextures(1, &mFramebufferID);
	glBindTexture(GL_TEXTURE_2D, mFramebufferID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	auto converterPixelFormat = LibretroToGLPixelFormat(pixelFormat);
	glTexImage2D(GL_TEXTURE_2D, 0, converterPixelFormat, requestedSize, requestedSize, 0, converterPixelFormat, GL_UNSIGNED_BYTE, nullptr);
}

void SoftwareCoreRenderer::RenderFrame(const Platform::Array<unsigned char, 1U> ^frameBuffer, unsigned int width, unsigned int height, unsigned int pitch)
{
	glBindTexture(GL_TEXTURE_2D, mFramebufferID);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, pitch);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, LibretroToGLPixelFormat(mFramebufferPixelFormat), GL_UNSIGNED_BYTE, frameBuffer->Data);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);
}

void SoftwareCoreRenderer::DeleteFramebuffer()
{
	if (mFramebufferID != NULL)
	{
		glDeleteTextures(1, &mFramebufferID);
	}

	mFramebufferID = NULL;
}

GLint SoftwareCoreRenderer::LibretroToGLPixelFormat(PixelFormats pixelFormat)
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