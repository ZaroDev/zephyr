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
#include <system_error>


#ifdef PLATFORM_WINDOWS

#include <dxgi1_6.h>

#include "D3D11Model.h"
#include <Zephyr/Asset/ModelImporter.h>

namespace Zephyr::D3D11::Core
{
	namespace
	{
		ComPtr<ID3D11Device> g_Device = nullptr;
		ComPtr<IDXGIFactory4> g_DxgiFactory = nullptr;
		ComPtr<ID3D11DeviceContext> g_DeviceContext = nullptr;
		ComPtr<IDXGISwapChain1> g_SwapChain = nullptr;
		ComPtr<ID3D11RenderTargetView> g_BackBuffer = nullptr;
		ComPtr<IDXGIAdapter1> g_Adapter = nullptr;

		std::vector<ComPtr<ID3D11Buffer>> g_Models{};
		ComPtr<ID3D11Buffer> g_TriangleVertexBuffer = nullptr;

		// TODO: Move to a geometry pass
		ComPtr<ID3D11Buffer> g_ConstantBuffer = nullptr;
		ComPtr<ID3D11RasterizerState> g_RastState;

#ifndef DIST
		ComPtr<ID3D11Debug> g_DebugLayer = nullptr;
#endif
		Ref<Model> g_Model;

		void GetHardwareAdapter(IDXGIAdapter1** ppAdapter)
		{
			*ppAdapter = nullptr;

			ComPtr<IDXGIAdapter1> adapter;

			ComPtr<IDXGIFactory6> factory6;
			HRESULT hr = g_DxgiFactory.As(&factory6);
			if (SUCCEEDED(hr))
			{
				for (UINT adapterIndex = 0;
					SUCCEEDED(factory6->EnumAdapterByGpuPreference(
						adapterIndex,
						DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
						IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
						adapterIndex++)
				{
					DXGI_ADAPTER_DESC1 desc;
					adapter->GetDesc1(&desc);

					if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
					{
						// Don't select the Basic Render Driver adapter.
						continue;
					}

#ifdef _DEBUG
					CORE_INFO("Direct3D Adapter ({0}): VID:{1}, PID:{2}X\n", adapterIndex, desc.VendorId, desc.DeviceId);
#endif

					break;
				}
			}

			if (!adapter)
			{
				for (UINT adapterIndex = 0;
					SUCCEEDED(g_DxgiFactory->EnumAdapters1(
						adapterIndex,
						adapter.ReleaseAndGetAddressOf()));
						adapterIndex++)
				{
					DXGI_ADAPTER_DESC1 desc;
					adapter->GetDesc1(&desc);

					if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
					{
						// Don't select the Basic Render Driver adapter.
						continue;
					}

#ifdef _DEBUG
					CORE_INFO("Direct3D Adapter ({0}): VID:{1}, PID:{2}X\n", adapterIndex, desc.VendorId, desc.DeviceId);
#endif

					break;
				}
			}

			*ppAdapter = adapter.Detach();
		}
	}

	extern "C"
	{
		__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
		__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
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
		
		GetHardwareAdapter(g_Adapter.GetAddressOf());
		
		

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

		g_Adapter->GetParent(IID_PPV_ARGS(&g_DxgiFactory));

		const auto& windowData = Window::GetWindowData();
		DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor = {};
		swapChainDescriptor.Width = windowData.Width;
		swapChainDescriptor.Height = windowData.Height;
		swapChainDescriptor.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDescriptor.SampleDesc.Count = 1;
		swapChainDescriptor.SampleDesc.Quality = 0;
		swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDescriptor.BufferCount = 2;
		swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD;
		swapChainDescriptor.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH;
		swapChainDescriptor.Flags = {};

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDescriptor = {};
		swapChainFullscreenDescriptor.Windowed = !windowData.Fullscreen;
		HRESULT hr = g_DxgiFactory->CreateSwapChainForHwnd(
			g_Device.Get(),
			static_cast<HWND>(Window::GetOSWindowPointer()),
			&swapChainDescriptor,
			&swapChainFullscreenDescriptor,
			nullptr,
			&g_SwapChain);
		if (FAILED(hr))
		{
			CORE_ERROR("DXGI: Failed to create swap chain! {0}", std::system_category().message(hr));
			CORE_ERROR("DXGI: Remove reason {0}", std::system_category().message(g_Device->GetDeviceRemovedReason()));

			return false;
		}

		if (!CreateSwapChainResources())
		{
			CORE_ERROR("D3D11: Unable to create swap chain resources!");
			return false;
		}

		CreateVertexBuffer();

		// Create a new rasterizer state
		D3D11_RASTERIZER_DESC desc = {};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK; // Enable back-facing culling
		desc.FrontCounterClockwise = true;
		desc.DepthBias = 0;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.MultisampleEnable = true;
		desc.AntialiasedLineEnable = false;
		g_Device->CreateRasterizerState(&desc, &g_RastState);



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
		g_Model = Zephyr::ModelImporter::LoadModel("Resources/rubber_duck/scene.gltf");

		{
			SceneConstantBuffer buffer{};

			D3D11_BUFFER_DESC desc{
				.ByteWidth = sizeof(SceneConstantBuffer),
				.Usage = D3D11_USAGE_DEFAULT,
			};
			desc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;

			D3D11_SUBRESOURCE_DATA resourceData = {};
			resourceData.pSysMem = &buffer;
			resourceData.SysMemPitch = 0;
			resourceData.SysMemSlicePitch = 0;

			HRESULT hr = Core::Device().CreateBuffer(&desc, &resourceData, &g_ConstantBuffer);
			if (FAILED(hr))
			{
				CORE_ERROR("D3D11: Failed to create scene constant buffer: {}", Win32ErrorMessage(hr));
			}
		}
	}
	bool CreateSwapChainResources()
	{
		ComPtr<ID3D11Texture2D> backBuffer = nullptr;
		HRESULT hr = g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		if (FAILED(hr))
		{
			CORE_ERROR("D3D11: Failed to get back buffer from swap chain! {0}", Win32ErrorMessage(hr));
			return false;
		}
		hr = g_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, &g_BackBuffer);
		if (FAILED(hr))
		{
			CORE_ERROR("D3D11: Failed to create RTV from back buffer!");
			return false;
		}

		return true;
	}
	void DestroySwapChainResources()
	{
		g_BackBuffer.Reset();
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
		ComPtr<IDXGIAdapter3> adapter;
		g_Adapter.As(&adapter);

		adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo);

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

	void BeginFrame( Camera& camera)
	{
		const auto& windowData = Window::GetWindowData();
		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Height = static_cast<FLOAT>(windowData.Height);
		viewport.Width = static_cast<FLOAT>(windowData.Width);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		camera.OnResize(windowData.Height, windowData.Width);
		constexpr f32 clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
		g_DeviceContext->OMSetRenderTargets(1, g_BackBuffer.GetAddressOf(), g_DepthBuffer.Get());
		
		g_DeviceContext->ClearRenderTargetView(g_BackBuffer.Get(), clearColor);
		
		g_DeviceContext->RSSetState(g_RastState.Get());
		g_DeviceContext->RSSetViewports(1, &viewport);
		
		Ref<D3D11Shader> shader = Cast<D3D11Shader>(Renderer::GetShaderLibrary().Get("main"));

		g_DeviceContext->IASetInputLayout(shader->GetLayout().Get());

	
		g_DeviceContext->VSSetShader(shader->GetVertex().Get(), nullptr, 0);
		g_DeviceContext->PSSetShader(shader->GetPixel().Get(), nullptr, 0);

		Mat4 model = Mat4(1.0);
		model = glm::rotate(model, 90.f, V3{0.f, 1.0f, 0.0f});
		SceneConstantBuffer constantBuffer
		{
			.Model = model,
			.View = camera.GetView(),
			.Projection = camera.GetProjection(),
		};

		D3D11_MAPPED_SUBRESOURCE subresource{};
		g_DeviceContext->Map(g_ConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
		memcpy(subresource.pData, &constantBuffer, sizeof(SceneConstantBuffer));
		g_DeviceContext->Unmap(g_ConstantBuffer.Get(), 0);

		g_DeviceContext->VSSetConstantBuffers(0, 1, g_ConstantBuffer.GetAddressOf());

		g_Model->Draw(0);
		
	}

	void EndFrame()
	{
		g_SwapChain->Present(1, 0);
	}
}
#endif