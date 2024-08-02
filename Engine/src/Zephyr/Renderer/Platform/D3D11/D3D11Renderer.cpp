#include <pch.h>
#include "D3D11Renderer.h"
#include <Zephyr/Core/Application.h>
#include "D3D11Shader.h"

#ifdef PLATFORM_WINDOWS

namespace Zephyr::D3D11
{
	bool D3D11Renderer::Init()
	{
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&m_DxgiFactory))))
		{
			CORE_ERROR("DXGI: Unable to create DXGIFactory");
			return false;
		}
		constexpr D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;
		if (FAILED(D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			&deviceFeatureLevel,
			1,
			D3D11_SDK_VERSION,
			&m_Device,
			nullptr,
			&m_DeviceContext
		)))
		{
			CORE_ERROR("D3D11: Failed to create device and device context!");
			return false;
		}
		const Window& window = Application::Get().GetWindow();

		DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor = {};
		swapChainDescriptor.Width = window.Data().Width;
		swapChainDescriptor.Height = window.Data().Height;
		swapChainDescriptor.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDescriptor.SampleDesc.Count = 1;
		swapChainDescriptor.SampleDesc.Quality = 0;
		swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDescriptor.BufferCount = 2;
		swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDescriptor.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH;
		swapChainDescriptor.Flags = {};

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDescriptor = {};
		swapChainFullscreenDescriptor.Windowed = !window.Data().Fullscreen;

		if (FAILED(m_DxgiFactory->CreateSwapChainForHwnd(
			m_Device.Get(),
			reinterpret_cast<HWND>(window.OSWindow()),
			&swapChainDescriptor,
			&swapChainFullscreenDescriptor,
			nullptr,
			&m_SwapChain
		)))
		{
			CORE_ERROR("DXGI: Failed to create swap chain!");
			return false;
		}

		if (!CreateSwapChainResources())
		{
			CORE_ERROR("D3D11: Unable to create swap chain resources!");
			return false;
		}

		CORE_INFO("D3D11: Renderer initialized!");

		return true;
	}
	void D3D11Renderer::Shutdown()
	{
		CORE_INFO("D3D11: Renderer closed!");
	}
	bool D3D11Renderer::CreateSwapChainResources()
	{
		ComPtr<ID3D11Texture2D> backBuffer = nullptr;
		if (FAILED(m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
		{
			CORE_ERROR("D3D11: Failed to get back buffer from swap chain!");
			return false;
		}

		if (FAILED(m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_RenderTarget)))
		{
			CORE_ERROR("D3D11: Failed to create RTV from back buffer!");
			return false;
		}

		return true;
	}
	void D3D11Renderer::DestroySwapChainResources()
	{
		m_RenderTarget.Reset();
	}

	void D3D11Renderer::OnResize(i32 width, i32 height)
	{
		m_DeviceContext->Flush();
		DestroySwapChainResources();

		if (FAILED(m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM, 0)))
		{
			CORE_ERROR("D3D11: Failed to recreate swap chain buffers!");
			return;
		}

		CreateSwapChainResources();
	}
	void D3D11Renderer::Render()
	{
		const Window& window = Application::Get().GetWindow();
		
		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Height = static_cast<FLOAT>(window.Data().Height);
		viewport.Width = static_cast<FLOAT>(window.Data().Width);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		constexpr f32 clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };

		m_DeviceContext->ClearRenderTargetView(m_RenderTarget.Get(), clearColor);
		m_DeviceContext->RSSetViewports(1, &viewport);
		m_DeviceContext->OMSetRenderTargets(1, m_RenderTarget.GetAddressOf(), nullptr);
		m_SwapChain->Present(1, 0);
	}
}
#endif