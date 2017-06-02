#pragma once

#include "OpenGLES.h"

using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

using namespace LibretroRT;

namespace LibretroRT_AngleRenderer
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class AngleRenderer sealed
	{
	public:
		AngleRenderer(SwapChainPanel^ swapChainPanel);
		virtual ~AngleRenderer();
		void StartRenderer(ICore^ core);
		void StopRenderer();

	private:
		void OnPageLoaded(Platform::Object^ sender, RoutedEventArgs^ e);
		void OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args);
		void CreateRenderSurface();
		void DestroyRenderSurface();
		void RecoverFromLostDevice();
		void StartRenderer();

		OpenGLES& mOpenGLES;

		SwapChainPanel^ mSwapChainPanel;
		EGLSurface mRenderSurface;     // This surface is associated with a swapChainPanel on the page

		ICore^ mCore;

		Concurrency::critical_section mRenderSurfaceCriticalSection;
		IAsyncAction^ mRenderLoopWorker;
	};
}