#include <pch.h>
#include "D3D11Renderer.h"
#include <Zephyr/Core/Application.h>
#include "D3D11Shader.h"

#include <imgui.h>
#include <ImGui/imgui_impl_dx11.h>
#include <ImGui/imgui_impl_glfw.h>

#include <Zephyr/Renderer/Window.h>

#include <locale>
#include <codecvt>
 


#ifdef PLATFORM_WINDOWS

#include <dxgi1_4.h>

namespace Zephyr::D3D11::Core
{
	namespace
	{
		ComPtr<ID3D11Device> g_Device = nullptr;
		ComPtr<IDXGIFactory4> g_DxgiFactory = nullptr;
		ComPtr<ID3D11DeviceContext> g_DeviceContext = nullptr;
		ComPtr<IDXGISwapChain1> g_SwapChain = nullptr;
		ComPtr<ID3D11RenderTargetView> g_RenderTarget = nullptr;
		IDXGIAdapter3* g_Adapter = nullptr;

		std::vector<ComPtr<ID3D11Buffer>> g_Models{};
		ComPtr<ID3D11Buffer> g_TriangleVertexBuffer = nullptr;

#ifndef DIST
		ComPtr<ID3D11Debug> g_DebugLayer = nullptr;
#endif
	}

	bool Init()
	{
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&g_DxgiFactory))))
		{
			CORE_ERROR("DXGI: Unable to create DXGIFactory");
			return false;
		}
		constexpr D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0;

		UINT deviceFlags = D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifndef DIST
		deviceFlags |= D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;
#endif
		
		g_DxgiFactory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter**>(&g_Adapter));

		
		

		if (FAILED(D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			deviceFlags,
			&deviceFeatureLevel,
			1,
			D3D11_SDK_VERSION,
			&g_Device,
			nullptr,
			&g_DeviceContext
		)))
		{
			CORE_ERROR("D3D11: Failed to create device and device context!");
			return false;
		}

#ifndef DIST
		if (FAILED(g_Device.As(&g_DebugLayer)))
		{
			CORE_ERROR("D3D11: Failed to get the debug layer from the device");
			return false;
		}
#endif


		const auto& windowData = Window::GetWindowData();
		DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor = {};
		swapChainDescriptor.Width = windowData.Width;
		swapChainDescriptor.Height = windowData.Height;
		swapChainDescriptor.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDescriptor.SampleDesc.Count = 1;
		swapChainDescriptor.SampleDesc.Quality = 0;
		swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDescriptor.BufferCount = 2;
		swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDescriptor.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH;
		swapChainDescriptor.Flags = {};

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDescriptor = {};
		swapChainFullscreenDescriptor.Windowed = !windowData.Fullscreen;

		if (FAILED(g_DxgiFactory->CreateSwapChainForHwnd(
			g_Device.Get(),
			static_cast<HWND>(Window::GetOSWindowPointer()),
			&swapChainDescriptor,
			&swapChainFullscreenDescriptor,
			nullptr,
			&g_SwapChain
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

		CreateVertexBuffer();

		CORE_INFO("D3D11: Renderer initialized!");

		return true;
	}
	void Shutdown()
	{
		CORE_INFO("D3D11: Renderer closed!");

		g_DeviceContext.Reset();

#ifndef DIST
		g_DebugLayer->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL);
		g_DebugLayer.Reset();
#endif

		g_Device.Reset();
	}
	void CreateTexture(D3D11Texture2D& texture, Buffer buffer)
	{
		D3D11_TEXTURE2D_DESC desc{};
		ZeroMemory(&desc, sizeof(desc));

		desc.Width = texture.GetWidth();
		desc.Height = texture.GetHeight();
		desc.MipLevels = 1;
		desc.ArraySize = 1;

		switch (texture.GetSpecification().Format)
		{
		case ImageFormat::R8: desc.Format = DXGI_FORMAT_R8_UNORM; break;
		case ImageFormat::RGB8: desc.Format = DXGI_FORMAT_R8G8B8A8_UINT; break;
		case ImageFormat::RGBA8: desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
		case ImageFormat::RGBA32F: desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
		}

		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA imageSubresouceData{};
		ZeroMemory(&imageSubresouceData, sizeof(imageSubresouceData));
		imageSubresouceData.pSysMem = buffer.Data;
		imageSubresouceData.SysMemPitch = texture.GetWidth() * texture.GetSpecification().Channels;

		ComPtr<ID3D11Texture2D> imageTexture;

		if (FAILED(g_Device->CreateTexture2D(&desc, &imageSubresouceData, &imageTexture)))
		{
			CORE_ERROR("D3D11: Failed to create texture 2d");
			return;
		}

		if (FAILED(g_Device->CreateShaderResourceView(imageTexture.Get(), nullptr, &texture.Texture)))
		{
			CORE_ERROR("D3D11: Failed to create shader resource view");
			return;
		}

		D3D11_SAMPLER_DESC samplerDesc{};
		ZeroMemory(&samplerDesc, sizeof(samplerDesc));

		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.BorderColor[0] = 1.0f;
		samplerDesc.BorderColor[1] = 1.0f;
		samplerDesc.BorderColor[2] = 1.0f;
		samplerDesc.BorderColor[3] = 1.0f;
		samplerDesc.MinLOD = -FLT_MAX;
		samplerDesc.MaxLOD = FLT_MAX;

		if (FAILED(g_Device->CreateSamplerState(&samplerDesc, &texture.m_SamplerState)))
		{
			CORE_ERROR("D3D11: Failed to crate sampler state");
		}
	}
	bool InitImGui()
	{
		ImGui_ImplGlfw_InitForOther(Window::GetGLFWWindow(), true);
		ImGui_ImplDX11_Init(g_Device.Get(), g_DeviceContext.Get());

		return true;
	}
	void ImGuiNewFrame()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplGlfw_NewFrame();
	}
	void ImGuiEndFrame()
	{
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
	void ShutdownImGui()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplGlfw_Shutdown();
	}
	void CreateVertexBuffer()
	{
		constexpr VertexPositionColor vertices[] =
		{
			{ V3{  0.0f,  0.5f, 0.0f }, Color3{ 1.0f, 0.0f, 0.0f } },
			{ V3{  0.5f, -0.5f, 0.0f }, Color3{ 0.0f, 1.0f, 0.0f } },
			{ V3{ -0.5f, -0.5f, 0.0f }, Color3{ 0.0f, 0.0f, 1.0f } },
		};

		D3D11_BUFFER_DESC bufferInfo = {};
		bufferInfo.ByteWidth = sizeof(vertices);
		bufferInfo.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
		bufferInfo.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA resourceData = {};
		resourceData.pSysMem = vertices;

		if (FAILED(g_Device->CreateBuffer(&bufferInfo, &resourceData, &g_TriangleVertexBuffer)))
		{
			CORE_ERROR("D3D11: Failed to create triangle vertex buffer");
		}
	}
	bool CreateSwapChainResources()
	{
		ComPtr<ID3D11Texture2D> backBuffer = nullptr;
		if (FAILED(g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
		{
			CORE_ERROR("D3D11: Failed to get back buffer from swap chain!");
			return false;
		}

		if (FAILED(g_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, &g_RenderTarget)))
		{
			CORE_ERROR("D3D11: Failed to create RTV from back buffer!");
			return false;
		}

		return true;
	}
	void DestroySwapChainResources()
	{
		g_RenderTarget.Reset();
	}

	ID3D11Device& Device()
	{
		return *g_Device.Get();
	}

	ID3D11DeviceContext& DeviceContext()
	{
		return *g_DeviceContext.Get();
	}

	NODISCARD RenderDevice GetRenderDevice()
	{
		DXGI_ADAPTER_DESC desc;
		g_Adapter->GetDesc(&desc);

		DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
		g_Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);

		RenderDevice device;
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		device.Name = converter.to_bytes(desc.Description);
		device.Vendor = std::to_string(desc.VendorId);
		device.AvailableVRAM = videoMemoryInfo.Budget / 1024 / 1024;
		device.UsedVRAM = videoMemoryInfo.CurrentUsage / 1024 / 1024;
		device.TotalVRAM = desc.DedicatedVideoMemory / 1024 / 1024;
		return device;
	}

	void OnResize(i32 width, i32 height)
	{
		g_DeviceContext->Flush();
		DestroySwapChainResources();

		if (FAILED(g_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM, 0)))
		{
			CORE_ERROR("D3D11: Failed to recreate swap chain buffers!");
			return;
		}

		CreateSwapChainResources();
	}

	void BeginFrame()
	{
		const auto& windowData = Window::GetWindowData();
		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Height = static_cast<FLOAT>(windowData.Height);
		viewport.Width = static_cast<FLOAT>(windowData.Width);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		constexpr f32 clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
		g_DeviceContext->OMSetRenderTargets(1, g_RenderTarget.GetAddressOf(), nullptr);
		g_DeviceContext->ClearRenderTargetView(g_RenderTarget.Get(), clearColor);


		g_DeviceContext->RSSetViewports(1, &viewport);
		Ref<D3D11Shader> shader = Cast<D3D11Shader>(Renderer::GetShaderLibrary().Get("main"));

		g_DeviceContext->IASetInputLayout(shader->Layout().Get());

		UINT vertexStride = sizeof(VertexPositionColor);
		UINT vertexOffset = 0;

		g_DeviceContext->IASetVertexBuffers(0, 1, g_TriangleVertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);

		g_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		g_DeviceContext->VSSetShader(shader->Vertex().Get(), nullptr, 0);
		g_DeviceContext->PSSetShader(shader->Pixel().Get(), nullptr, 0);



		g_DeviceContext->Draw(3, 0);
	}

	void EndFrame()
	{
		g_SwapChain->Present(1, 0);
	}
}
#endif