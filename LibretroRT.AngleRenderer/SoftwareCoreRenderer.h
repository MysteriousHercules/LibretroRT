#pragma once

#include "OpenGLES.h"

using namespace LibretroRT;

namespace LibretroRT_AngleRenderer
{
	class SoftwareCoreRenderer
	{
	public:
		SoftwareCoreRenderer();
		~SoftwareCoreRenderer();
		void UpdateFramebufferFormat(GameGeometry^ geometry, PixelFormats pixelFormat);
		void RenderFrame(const Platform::Array<unsigned char, 1U> ^frameBuffer, unsigned int width, unsigned int height, unsigned int pitch);

	private:
		static const unsigned TextureMinSize;
		static GLint LibretroToGLPixelFormat(PixelFormats pixelFormat);

		void DeleteFramebuffer();

		GLuint mFramebufferID;
		unsigned mFramebufferSize;
		PixelFormats mFramebufferPixelFormat;
	};
}

