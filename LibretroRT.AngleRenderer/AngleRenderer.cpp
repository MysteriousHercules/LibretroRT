#include "pch.h"
#include "AngleRenderer.h"

using namespace LibretroRT_AngleRenderer;
using namespace Platform;
using namespace Concurrency;
using namespace Windows::Foundation;
using namespace Windows::Storage;

AngleRenderer::AngleRenderer(SwapChainPanel^ swapChainPanel) :
	mOpenGLES(*OpenGLES::GetInstance()),
	mSwapChainPanel(swapChainPanel),
	mRenderSurface(EGL_NO_SURFACE),
	mCore(nullptr)
{
	Windows::UI::Core::CoreWindow^ window = Windows::UI::Xaml::Window::Current->CoreWindow;

	window->VisibilityChanged += ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::VisibilityChangedEventArgs^>(this, &AngleRenderer::OnVisibilityChanged);

	mSwapChainPanel->Loaded += ref new Windows::UI::Xaml::RoutedEventHandler(this, &AngleRenderer::OnPageLoaded);
}

AngleRenderer::~AngleRenderer()
{
	StopRendering();
	DestroyRenderSurface();
}

void AngleRenderer::OnPageLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	// The SwapChainPanel has been created and arranged in the page layout, so EGL can be initialized.
	CreateRenderSurface();
}

void AngleRenderer::OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args)
{
	if (args->Visible && mRenderSurface != EGL_NO_SURFACE)
	{
		StartRendering();
	}
	else
	{
		StopRendering();
	}
}

void LibretroRT_AngleRenderer::AngleRenderer::OnPixelFormatChanged(LibretroRT::PixelFormats format)
{
	throw ref new Platform::NotImplementedException();
}

void LibretroRT_AngleRenderer::AngleRenderer::OnGameGeometryChanged(LibretroRT::GameGeometry ^geometry)
{
	throw ref new Platform::NotImplementedException();
}

void LibretroRT_AngleRenderer::AngleRenderer::OnRenderVideoFrame(const Platform::Array<unsigned char, 1U> ^frameBuffer, unsigned int width, unsigned int height, unsigned int pitch)
{
	throw ref new Platform::NotImplementedException();
}

void AngleRenderer::CreateRenderSurface()
{
	if (mRenderSurface == EGL_NO_SURFACE)
	{
		// The app can configure the the SwapChainPanel which may boost performance.
		// By default, this template uses the default configuration.
		mRenderSurface = mOpenGLES.CreateSurface(mSwapChainPanel, nullptr, nullptr);

		// You can configure the SwapChainPanel to render at a lower resolution and be scaled up to
		// the swapchain panel size. This scaling is often free on mobile hardware.
		//
		// One way to configure the SwapChainPanel is to specify precisely which resolution it should render at.
		// Size customRenderSurfaceSize = Size(800, 600);
		// mRenderSurface = mOpenGLES->CreateSurface(swapChainPanel, &customRenderSurfaceSize, nullptr);
		//
		// Another way is to tell the SwapChainPanel to render at a certain scale factor compared to its size.
		// e.g. if the SwapChainPanel is 1920x1280 then setting a factor of 0.5f will make the app render at 960x640
		// float customResolutionScale = 0.5f;
		// mRenderSurface = mOpenGLES->CreateSurface(swapChainPanel, nullptr, &customResolutionScale);
		// 
	}
}

void AngleRenderer::DestroyRenderSurface()
{
	mOpenGLES.DestroySurface(mRenderSurface);
	mRenderSurface = EGL_NO_SURFACE;
}

void AngleRenderer::RecoverFromLostDevice()
{
	// Stop the render loop, reset OpenGLES, recreate the render surface
	// and start the render loop again to recover from a lost device.

	StopRendering();
	DestroyRenderSurface();
	mOpenGLES.Reset();
	CreateRenderSurface();
	StartRendering();
}

void AngleRenderer::StartCore(ICore^ core, IStorageFile^ gameFile)
{
	StopRendering();
	StopCore();

	mCore = core;
	mPixelFormatChangedRegistrationToken = mCore->PixelFormatChanged += ref new LibretroRT::PixelFormatChangedDelegate(this, &LibretroRT_AngleRenderer::AngleRenderer::OnPixelFormatChanged);
	mGameGeometryChangedRegistrationToken= mCore->GameGeometryChanged += ref new LibretroRT::GameGeometryChangedDelegate(this, &LibretroRT_AngleRenderer::AngleRenderer::OnGameGeometryChanged);
	mRenderVideoFrameRegistrationToken = mCore->RenderVideoFrame += ref new LibretroRT::RenderVideoFrameDelegate(this, &LibretroRT_AngleRenderer::AngleRenderer::OnRenderVideoFrame);
	mCore->LoadGame(gameFile);

	StartRendering();
}

void AngleRenderer::StopCore()
{
	StopRendering();
	if (mCore = nullptr)
	{
		return;
	}

	mCore->UnloadGame();
	mCore->PixelFormatChanged -= mPixelFormatChangedRegistrationToken;
	mCore->GameGeometryChanged -= mGameGeometryChangedRegistrationToken;
	mCore->RenderVideoFrame -= mRenderVideoFrameRegistrationToken;
}

void AngleRenderer::StartRendering()
{
	// If the render loop is already running then do not start another thread.
	if (mRenderLoopWorker != nullptr && mRenderLoopWorker->Status == Windows::Foundation::AsyncStatus::Started)
	{
		return;
	}

	// Create a task for rendering that will be run on a background thread.
	auto workItemHandler = ref new Windows::System::Threading::WorkItemHandler([this](Windows::Foundation::IAsyncAction ^ action)
	{
		critical_section::scoped_lock lock(mRenderSurfaceCriticalSection);

		mOpenGLES.MakeCurrent(mRenderSurface);

		while (action->Status == Windows::Foundation::AsyncStatus::Started)
		{
			EGLint panelWidth = 0;
			EGLint panelHeight = 0;
			mOpenGLES.GetSurfaceDimensions(mRenderSurface, &panelWidth, &panelHeight);

			// Logic to update the scene could go here
			mCore->RunFrame();

			// The call to eglSwapBuffers might not be successful (i.e. due to Device Lost)
			// If the call fails, then we must reinitialize EGL and the GL resources.
			if (mOpenGLES.SwapBuffers(mRenderSurface) != GL_TRUE)
			{
				// XAML objects like the SwapChainPanel must only be manipulated on the UI thread.
				mSwapChainPanel->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::High, ref new Windows::UI::Core::DispatchedHandler([=]()
				{
					RecoverFromLostDevice();
				}, CallbackContext::Any));

				return;
			}
		}
	});

	// Run task on a dedicated high priority background thread.
	mRenderLoopWorker = Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, Windows::System::Threading::WorkItemPriority::High, Windows::System::Threading::WorkItemOptions::TimeSliced);
}

void AngleRenderer::StopRendering()
{
	if (mRenderLoopWorker == nullptr)
	{
		return;
	}

	mRenderLoopWorker->Cancel();

	//Only return after cancellation is complete
	{
		critical_section::scoped_lock lock(mRenderSurfaceCriticalSection);
		mRenderLoopWorker = nullptr;
	}
}
