#pragma once

#include <Windows.h>
#include <dxgi1_3.h>

#include <nvrhi/d3d11.h>

#include <Zephyr/RHI/Surface.h>
#include <Zephyr/Window/Window.h>

namespace Zephyr
{
	class D3D11Device;

	class D3D11Surface final : public Surface
	{
	public:
		D3D11Surface(D3D11Device* device, const Ref<Window>& window);
		~D3D11Surface();

		// D3D11's implicit back buffer model has no app-visible frame index - always 0.
		virtual u32 GetCurrentFrameIndex() const override { return 0; }
		virtual u32 GetWidth() const override { return m_Width; }
		virtual u32 GetHeight() const override { return m_Height; }
		virtual void Resize(u32 width, u32 height) override;

		virtual bool AcquireNextImage() override;
		virtual nvrhi::IFramebuffer* GetCurrentFramebuffer() override;
		virtual bool Present() override;

	private:
		bool CreateSwapChain();
		void DestroySwapChain();
		bool CreateRenderTarget();
		void ReleaseRenderTarget();

		D3D11Device* m_Device = nullptr;
		Ref<Window> m_Window;
		HWND m_hWnd = nullptr;

		u32 m_Width = 0;
		u32 m_Height = 0;

		nvrhi::RefCountPtr<IDXGISwapChain> m_SwapChain;
		DXGI_SWAP_CHAIN_DESC m_SwapChainDesc{};

		nvrhi::RefCountPtr<ID3D11Texture2D> m_D3D11BackBuffer;
		nvrhi::TextureHandle m_RhiBackBuffer;
		nvrhi::FramebufferHandle m_Framebuffer;
	};
}
