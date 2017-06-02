#include "pch.h"
#include "AngleRenderer.h"

using namespace LibretroRT_AngleRenderer;
using namespace Platform;
using namespace Concurrency;
using namespace Windows::Foundation;

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
	StopRenderer();
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
		StartRenderer();
	}
	else
	{
		StopRenderer();
	}
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

	StopRenderer();

	{
		critical_section::scoped_lock lock(mRenderSurfaceCriticalSection);

		DestroyRenderSurface();
		mOpenGLES.Reset();
		CreateRenderSurface();
	}

	StartRenderer();
}

void AngleRenderer::StartRenderer(ICore^ core)
{
	StopRenderer();

	{
		critical_section::scoped_lock lock(mRenderSurfaceCriticalSection);

		mCore = core;
	}

	StartRenderer();
}

void AngleRenderer::StartRenderer()
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

		mRenderer->Init();

		while (action->Status == Windows::Foundation::AsyncStatus::Started)
		{
			EGLint panelWidth = 0;
			EGLint panelHeight = 0;
			mOpenGLES.GetSurfaceDimensions(mRenderSurface, &panelWidth, &panelHeight);

			// Logic to update the scene could go here
			mRenderer->UpdateWindowSize(panelWidth, panelHeight);
			mRenderer->Draw();

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

		mRenderer->Deinit();
	});

	// Run task on a dedicated high priority background thread.
	mRenderLoopWorker = Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, Windows::System::Threading::WorkItemPriority::High, Windows::System::Threading::WorkItemOptions::TimeSliced);
}

void AngleRenderer::StopRenderer()
{
	if (mRenderLoopWorker)
	{
		mRenderLoopWorker->Cancel();
		mRenderLoopWorker = nullptr;
	}
}