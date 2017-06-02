#pragma once

#include "OpenGLES.h"

using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Storage;

using namespace LibretroRT;

namespace LibretroRT_AngleRenderer
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class AngleRenderer sealed
	{
	public:
		AngleRenderer(SwapChainPanel^ swapChainPanel);
		virtual ~AngleRenderer();
		void StartCore(ICore^ core, IStorageFile^ gameFile);
		void StopCore();

	private:
		void OnPageLoaded(Platform::Object^ sender, RoutedEventArgs^ e);
		void OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args);
		void OnPixelFormatChanged(PixelFormats format);
		void OnGameGeometryChanged(LibretroRT::GameGeometry ^geometry);
		void OnRenderVideoFrame(const Platform::Array<unsigned char, 1U> ^frameBuffer, unsigned int width, unsigned int height, unsigned int pitch);

		void CreateRenderSurface();
		void DestroyRenderSurface();
		void RecoverFromLostDevice();
		void StartRendering();
		void StopRendering();

		ICore^ mCore;

		OpenGLES& mOpenGLES;
		SwapChainPanel^ mSwapChainPanel;
		EGLSurface mRenderSurface;     // This surface is associated with a swapChainPanel on the page

		Concurrency::critical_section mRenderSurfaceCriticalSection;
		IAsyncAction^ mRenderLoopWorker;

		EventRegistrationToken mPixelFormatChangedRegistrationToken;
		EventRegistrationToken mGameGeometryChangedRegistrationToken;
		EventRegistrationToken mRenderVideoFrameRegistrationToken;
	};
}