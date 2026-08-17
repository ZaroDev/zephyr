#pragma once

#include <Windows.h>
#include <dxgi1_4.h>

#include <nvrhi/d3d12.h>

#include <Zephyr/RHI/Surface.h>
#include <Zephyr/Window/Window.h>

namespace Zephyr
{
	class D3D12Device;

	class D3D12Surface final : public Surface
	{
	public:
		D3D12Surface(D3D12Device* device, const Ref<Window>& window);
		~D3D12Surface();

		virtual u32 GetCurrentFrameIndex() const override { return m_BackBufferIndex; }
		virtual u32 GetWidth() const override { return m_Width; }
		virtual u32 GetHeight() const override { return m_Height; }
		virtual void Resize(u32 width, u32 height) override;

		virtual bool AcquireNextImage() override;
		virtual nvrhi::IFramebuffer* GetCurrentFramebuffer() override;
		virtual bool Present() override;

	private:
		bool CreateSwapChain();
		void DestroySwapChain();
		bool CreateRenderTargets();
		void ReleaseRenderTargets();
		void WaitForGPU();

		D3D12Device* m_Device = nullptr;
		Ref<Window> m_Window;
		HWND m_hWnd = nullptr;

		u32 m_Width = 0;
		u32 m_Height = 0;

		nvrhi::RefCountPtr<IDXGISwapChain3> m_SwapChain;
		u32 m_BackBufferIndex = 0;

		std::vector<nvrhi::RefCountPtr<ID3D12Resource>> m_D3D12BackBuffers;
		std::vector<nvrhi::TextureHandle> m_RhiBackBuffers;
		std::vector<nvrhi::FramebufferHandle> m_Framebuffers;

		// Per-frame fence sync: D3D12's flip-model swap chain requires the app to explicitly
		// wait for the GPU to finish with a back buffer before reusing it - D3D11's driver did
		// this implicitly. One shared fence + one "last submitted value" per back buffer index,
		// matching the standard D3D12 sample pattern (MoveToNextFrame).
		nvrhi::RefCountPtr<ID3D12Fence> m_Fence;
		HANDLE m_FenceEvent = nullptr;
		u64 m_NextFenceValue = 1;
		std::vector<u64> m_FrameFenceValues;
	};
}
