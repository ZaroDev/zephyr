#pragma once

#include "D3D11Common.h"
#include <Zephyr/Renderer/RenderHardwareInterface.h>

#ifdef PLATFORM_WINDOWS

namespace Zephyr::D3D11
{
	class D3D11Renderer final : public RenderHardwareInterface
	{
	public:
		D3D11Renderer() : RenderHardwareInterface(){}
		~D3D11Renderer() = default;

		bool Init() override;
		void Shutdown() override;
		void OnResize(i32 width, i32 height) override;
		void Render() override;

		 ID3D11Device& Device() const { return *m_Device.Get(); }
	private:
		bool CreateSwapChainResources();
		void DestroySwapChainResources();

	private:
		ComPtr<ID3D11Device> m_Device = nullptr;
		ComPtr<IDXGIFactory2> m_DxgiFactory = nullptr;
		ComPtr<ID3D11DeviceContext> m_DeviceContext = nullptr;
		ComPtr<IDXGISwapChain1> m_SwapChain = nullptr;
		ComPtr<ID3D11RenderTargetView> m_RenderTarget = nullptr;
	};
}

#endif