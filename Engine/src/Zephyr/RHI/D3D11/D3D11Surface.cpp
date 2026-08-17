#include "pch.h"
#include "D3D11Surface.h"
#include "D3D11Device.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Zephyr
{
	static constexpr nvrhi::Format c_SwapChainFormat = nvrhi::Format::SBGRA8_UNORM;

	D3D11Surface::D3D11Surface(D3D11Device* device, const Ref<Window>& window)
		: m_Device(device), m_Window(window)
	{
		m_hWnd = glfwGetWin32Window(window->GetGLFWHandle());

		int w, h;
		glfwGetFramebufferSize(window->GetGLFWHandle(), &w, &h);
		m_Width = u32(w);
		m_Height = u32(h);

		if (!CreateSwapChain())
		{
			CORE_CRITICAL("Failed to create a D3D11 swap chain");
		}
	}

	D3D11Surface::~D3D11Surface()
	{
		DestroySwapChain();
	}

	bool D3D11Surface::CreateSwapChain()
	{
		DestroySwapChain();

		ZeroMemory(&m_SwapChainDesc, sizeof(m_SwapChainDesc));
		m_SwapChainDesc.BufferCount = 2; // minimum required by DXGI_SWAP_EFFECT_FLIP_DISCARD
		m_SwapChainDesc.BufferDesc.Width = m_Width;
		m_SwapChainDesc.BufferDesc.Height = m_Height;
		// DXGI will not create a swap chain with an sRGB format directly, even though its
		// contents are interpreted as sRGB - use the non-sRGB format here (matches
		// D3D11DeviceManager's special-casing for SBGRA8_UNORM/SRGBA8_UNORM).
		m_SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		m_SwapChainDesc.BufferUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		m_SwapChainDesc.OutputWindow = m_hWnd;
		m_SwapChainDesc.SampleDesc.Count = 1;
		m_SwapChainDesc.SampleDesc.Quality = 0;
		m_SwapChainDesc.Windowed = TRUE;
		m_SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		m_SwapChainDesc.Flags = 0;

		const HRESULT hr = m_Device->GetDxgiFactory()->CreateSwapChain(m_Device->GetD3D11Device(), &m_SwapChainDesc, &m_SwapChain);
		if (FAILED(hr))
		{
			CORE_ERROR("Failed to create a D3D11 swap chain, HRESULT = 0x{0:x}", static_cast<u32>(hr));
			return false;
		}

		return CreateRenderTarget();
	}

	void D3D11Surface::DestroySwapChain()
	{
		ReleaseRenderTarget();

		if (m_SwapChain)
		{
			m_SwapChain->SetFullscreenState(FALSE, nullptr);
		}

		m_SwapChain = nullptr;
	}

	bool D3D11Surface::CreateRenderTarget()
	{
		ReleaseRenderTarget();

		const HRESULT hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&m_D3D11BackBuffer);
		if (FAILED(hr))
		{
			CORE_ERROR("Failed to get the D3D11 swap chain back buffer, HRESULT = 0x{0:x}", static_cast<u32>(hr));
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

		m_RhiBackBuffer = m_Device->GetNvrhiDevice()->createHandleForNativeTexture(
			nvrhi::ObjectTypes::D3D11_Resource, static_cast<ID3D11Resource*>(m_D3D11BackBuffer.Get()), textureDesc);
		if (!m_RhiBackBuffer)
			return false;

		m_Framebuffer = m_Device->GetNvrhiDevice()->createFramebuffer(nvrhi::FramebufferDesc().addColorAttachment(m_RhiBackBuffer));

		return true;
	}

	void D3D11Surface::ReleaseRenderTarget()
	{
		m_Framebuffer = nullptr;
		m_RhiBackBuffer = nullptr;
		m_D3D11BackBuffer = nullptr;
	}

	void D3D11Surface::Resize(u32 width, u32 height)
	{
		if (width == m_Width && height == m_Height)
			return;

		m_Width = width;
		m_Height = height;

		ReleaseRenderTarget();

		if (!m_SwapChain)
			return;

		// BufferCount=0 and Format=UNKNOWN keep the existing values (DXGI convention).
		const HRESULT hr = m_SwapChain->ResizeBuffers(0, m_Width, m_Height, DXGI_FORMAT_UNKNOWN, m_SwapChainDesc.Flags);
		if (FAILED(hr))
		{
			CORE_ERROR("D3D11 ResizeBuffers failed, HRESULT = 0x{0:x}", static_cast<u32>(hr));
			return;
		}

		if (!CreateRenderTarget())
		{
			CORE_ERROR("Failed to recreate the D3D11 render target after resize");
		}
	}

	bool D3D11Surface::AcquireNextImage()
	{
		return m_Framebuffer != nullptr;
	}

	nvrhi::IFramebuffer* D3D11Surface::GetCurrentFramebuffer()
	{
		return m_Framebuffer;
	}

	bool D3D11Surface::Present()
	{
		const HRESULT hr = m_SwapChain->Present(0, 0);
		return SUCCEEDED(hr);
	}
}
