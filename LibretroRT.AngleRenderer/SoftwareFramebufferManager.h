#pragma once

#include "OpenGLES.h"

using namespace LibretroRT;

namespace LibretroRT_AngleRenderer
{
	public class SoftwareFramebufferManager
	{
	public:
		SoftwareFramebufferManager();
		~SoftwareFramebufferManager();
		GLuint TextureId();
		void UpdateTextureFormat(GameGeometry^ geometry, PixelFormats pixelFormat);

	private:
		static const unsigned TextureMinSize;
		static GLint LibretroToGLPixelFormat(PixelFormats pixelFormat);

		void DeleteTexture();

		GLuint mTextureID;
		unsigned mTextureSize;
		PixelFormats mTexturePixelFormat;
	};
}

