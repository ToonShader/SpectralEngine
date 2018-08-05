#pragma once

#include "Common\StepTimer.h"
//#include "Common\DeviceResources.h"
//#include "Content\Sample3DSceneRenderer.h"
//#include "Content\SampleFpsTextRenderer.h"

// Renders Direct2D and 3D content on the screen.
namespace SpectralEditor
{
	interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

	class SpectralEditorMain : public IDeviceNotify
	{
	public:
		SpectralEditorMain();
		~SpectralEditorMain();
		void CreateWindowSizeDependentResources();
		//void StartTracking() { m_sceneRenderer->StartTracking(); }
		void TrackingUpdate(float positionX) { m_pointerLocationX = positionX; }
		//void StopTracking() { m_sceneRenderer->StopTracking(); }
		//bool IsTracking() { return m_sceneRenderer->IsTracking(); }
		void StartRenderLoop(Windows::UI::Xaml::Controls::SwapChainPanel^ panel);
		void StopRenderLoop();
		Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		void ProcessInput();
		void Update();
		bool Render();

		// Cached pointer to device resources.
		//std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// TODO: Replace with your own content renderers.
		//std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;
		//std::unique_ptr<SampleFpsTextRenderer> m_fpsTextRenderer;

		Windows::Foundation::IAsyncAction^ m_renderLoopWorker;
		Concurrency::critical_section m_criticalSection;
		Windows::UI::Xaml::Controls::SwapChainPanel^ mPanel;

		// Rendering loop timer.
		DX::StepTimer m_timer;

		// Track current input pointer position.
		float m_pointerLocationX;
	};
}