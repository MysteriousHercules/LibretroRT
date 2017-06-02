﻿#pragma once

namespace LibretroRT_AngleRenderer
{
	class OpenGLES
	{
	public:
		~OpenGLES();

		static std::shared_ptr<OpenGLES>& GetInstance();
		EGLSurface CreateSurface(Windows::UI::Xaml::Controls::SwapChainPanel^ panel, const Windows::Foundation::Size* renderSurfaceSize, const float* renderResolutionScale);
		void GetSurfaceDimensions(const EGLSurface surface, EGLint *width, EGLint *height);
		void DestroySurface(const EGLSurface surface);
		void MakeCurrent(const EGLSurface surface);
		EGLBoolean SwapBuffers(const EGLSurface surface);
		void Reset();

	private:
		static std::mutex mMutex;

		OpenGLES();
		OpenGLES(const OpenGLES& rs);
		OpenGLES& operator = (const OpenGLES& rs);

		void Initialize();
		void Cleanup();

		EGLDisplay mEglDisplay;
		EGLContext mEglContext;
		EGLConfig  mEglConfig;
	};
}
