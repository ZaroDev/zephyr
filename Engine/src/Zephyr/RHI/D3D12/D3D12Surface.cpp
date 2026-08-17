#include "pch.h"
#include "D3D12Surface.h"
#include "D3D12Device.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Zephyr
{
	static constexpr nvrhi::Format c_SwapChainFormat = nvrhi::Format::SBGRA8_UNORM;

	D3D12Surface::D3D12Surface(D3D12Device* device, const Ref<Window>& window)
		: m_Device(device), m_Window(window)
	{
		m_hWnd = glfwGetWin32Window(window->GetGLFWHandle());

		int w, h;
		glfwGetFramebufferSize(window->GetGLFWHandle(), &w, &h);
		m_Width = u32(w);
		m_Height = u32(h);

		const HRESULT fenceHr = m_Device->GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
		if (FAILED(fenceHr))
		{
			CORE_CRITICAL("Failed to create a D3D12 fence, HRESULT = 0x{0:x}", static_cast<u32>(fenceHr));
			return;
		}

		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!m_FenceEvent)
		{
			CORE_CRITICAL("Failed to create the D3D12 fence event");
			return;
		}

		if (!CreateSwapChain())
		{
			CORE_CRITICAL("Failed to create a D3D12 swap chain");
		}
	}

	D3D12Surface::~D3D12Surface()
	{
		WaitForGPU();
		DestroySwapChain();

		if (m_FenceEvent)
		{
			CloseHandle(m_FenceEvent);
			m_FenceEvent = nullptr;
		}
	}

	bool D3D12Surface::CreateSwapChain()
	{
		DestroySwapChain();

		DXGI_SWAP_CHAIN_DESC1 desc{};
		desc.Width = m_Width;
		desc.Height = m_Height;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = c_FramesInFlight;
		desc.Scaling = DXGI_SCALING_STRETCH;
		// Flip-model, matching D3D11Surface's DXGI_SWAP_EFFECT_FLIP_DISCARD - D3D12 requires it.
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

		nvrhi::RefCountPtr<IDXGISwapChain1> swapChain1;
		HRESULT hr = m_Device->GetDxgiFactory()->CreateSwapChainForHwnd(
			m_Device->GetGraphicsQueue(), m_hWnd, &desc, nullptr, nullptr, &swapChain1);
		if (FAILED(hr))
		{
			CORE_ERROR("Failed to create a D3D12 swap chain, HRESULT = 0x{0:x}", static_cast<u32>(hr));
			return false;
		}

		hr = swapChain1->QueryInterface(IID_PPV_ARGS(&m_SwapChain));
		if (FAILED(hr))
		{
			CORE_ERROR("Failed to query IDXGISwapChain3, HRESULT = 0x{0:x}", static_cast<u32>(hr));
			return false;
		}

		m_Device->GetDxgiFactory()->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

		m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

		return CreateRenderTargets();
	}

	void D3D12Surface::DestroySwapChain()
	{
		ReleaseRenderTargets();
		m_SwapChain = nullptr;
	}

	bool D3D12Surface::CreateRenderTargets()
	{
		ReleaseRenderTargets();

		m_D3D12BackBuffers.resize(c_FramesInFlight);
		m_RhiBackBuffers.resize(c_FramesInFlight);
		m_Framebuffers.resize(c_FramesInFlight);
		m_FrameFenceValues.resize(c_FramesInFlight, 0);

		for (u32 i = 0; i < c_FramesInFlight; ++i)
		{
			const HRESULT hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_D3D12BackBuffers[i]));
			if (FAILED(hr))
			{
				CORE_ERROR("Failed to get D3D12 swap chain buffer {0}, HRESULT = 0x{1:x}", i, static_cast<u32>(hr));
				return false;
			}

			nvrhi::TextureDesc textureDesc;
			textureDesc.width = m_Width;
			textureDesc.height = m_Height;
			textureDesc.sampleCount = 1;
			textureDesc.sampleQuality = 0;
			textureDesc.format = c_SwapChainFormat;
			textureDesc.debugName = "SwapChainBuffer";
			textureDesc.isRenderTarget = true;
			textureDesc.isUAV = false;
			// Matches VulkanSurface's swap chain texture desc - the app never transitions this
			// out of Present itself, nvrhi's command list barrier handling does it around the
			// render-target use each frame.
			textureDesc.initialState = nvrhi::ResourceStates::Present;
			textureDesc.keepInitialState = true;

			m_RhiBackBuffers[i] = m_Device->GetNvrhiDevice()->createHandleForNativeTexture(
				nvrhi::ObjectTypes::D3D12_Resource, static_cast<ID3D12Resource*>(m_D3D12BackBuffers[i].Get()), textureDesc);
			if (!m_RhiBackBuffers[i])
				return false;

			m_Framebuffers[i] = m_Device->GetNvrhiDevice()->createFramebuffer(
				nvrhi::FramebufferDesc().addColorAttachment(m_RhiBackBuffers[i]));
		}

		return true;
	}

	void D3D12Surface::ReleaseRenderTargets()
	{
		m_Framebuffers.clear();
		m_RhiBackBuffers.clear();
		m_D3D12BackBuffers.clear();
	}

	void D3D12Surface::WaitForGPU()
	{
		if (!m_Fence || !m_Device->GetGraphicsQueue())
			return;

		const u64 fenceValue = m_NextFenceValue++;
		m_Device->GetGraphicsQueue()->Signal(m_Fence, fenceValue);

		if (m_Fence->GetCompletedValue() < fenceValue)
		{
			m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent);
			WaitForSingleObject(m_FenceEvent, INFINITE);
		}
	}

	void D3D12Surface::Resize(u32 width, u32 height)
	{
		if (width == m_Width && height == m_Height)
			return;

		// D3D12 back buffers can't be released/resized while the GPU may still be using them -
		// D3D11's driver handled this implicitly, D3D12 requires an explicit idle wait.
		WaitForGPU();

		m_Width = width;
		m_Height = height;

		ReleaseRenderTargets();

		if (!m_SwapChain)
			return;

		const HRESULT hr = m_SwapChain->ResizeBuffers(c_FramesInFlight, m_Width, m_Height, DXGI_FORMAT_UNKNOWN, 0);
		if (FAILED(hr))
		{
			CORE_ERROR("D3D12 ResizeBuffers failed, HRESULT = 0x{0:x}", static_cast<u32>(hr));
			return;
		}

		m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

		if (!CreateRenderTargets())
		{
			CORE_ERROR("Failed to recreate the D3D12 render targets after resize");
		}
	}

	bool D3D12Surface::AcquireNextImage()
	{
		if (!m_SwapChain)
			return false;

		m_BackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
		if (m_BackBufferIndex >= m_Framebuffers.size())
			return false;

		// Wait for the GPU to finish whatever previous frame last used this back buffer index -
		// D3D12 gives no implicit guarantee it's safe to render into otherwise.
		const u64 fenceValue = m_FrameFenceValues[m_BackBufferIndex];
		if (fenceValue != 0 && m_Fence->GetCompletedValue() < fenceValue)
		{
			m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent);
			WaitForSingleObject(m_FenceEvent, INFINITE);
		}

		return m_Framebuffers[m_BackBufferIndex] != nullptr;
	}

	nvrhi::IFramebuffer* D3D12Surface::GetCurrentFramebuffer()
	{
		if (m_BackBufferIndex < m_Framebuffers.size())
			return m_Framebuffers[m_BackBufferIndex];

		return nullptr;
	}

	bool D3D12Surface::Present()
	{
		const HRESULT hr = m_SwapChain->Present(0, 0);
		if (FAILED(hr))
			return false;

		const u64 fenceValue = m_NextFenceValue++;
		m_Device->GetGraphicsQueue()->Signal(m_Fence, fenceValue);
		m_FrameFenceValues[m_BackBufferIndex] = fenceValue;

		return true;
	}
}
