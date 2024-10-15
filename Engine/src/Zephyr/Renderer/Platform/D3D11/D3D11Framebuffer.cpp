#include <pch.h>
#include "D3D11Framebuffer.h"


#include "D3D11Renderer.h"

namespace Zephyr::D3D11
{
	namespace 
	{
		bool IsDepthFormat(FramebufferTextureFormat format)
		{
			switch (format)
			{
			case FramebufferTextureFormat::DEPTH24STENCIL8:  return true;
			}

			return false;
		}
		bool IsFormatSupported(DXGI_FORMAT format, u32 flag)
		{
			u32 support = 0;
			HRESULT hr = Core::Device().CheckFormatSupport(format, &support);
			if(FAILED(hr))
			{
				CORE_ERROR("D3D11: Failed to check format support {}", Win32ErrorMessage(hr));
				return false;
			}

			if (support & flag)
			{
				return true;
			}
			
			return false;
		}

		DXGI_FORMAT GetFormat(FramebufferTextureFormat format)
		{
			switch (format)
			{
			case FramebufferTextureFormat::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
			case FramebufferTextureFormat::RGBA16: return DXGI_FORMAT_R16G16B16A16_FLOAT;
			case FramebufferTextureFormat::RGBA32: return DXGI_FORMAT_R32G32B32_FLOAT; 
			case FramebufferTextureFormat::RED_INTEGER: return DXGI_FORMAT_R8_UINT; 
			}

			return c_DefaultFormat;
		}

		D3D11RenderTarget CreateColorRenderTarget(u32 width, u32 height, DXGI_FORMAT format, u32 samples)
		{
			D3D11RenderTarget result;

			const u32 usageFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			HRESULT hr = S_OK;
			format = IsFormatSupported(format, usageFlags) ? format : c_DefaultFormat;


			D3D11_TEXTURE2D_DESC textureDesc{};
			textureDesc.Width = width;
			textureDesc.Height = height;
			textureDesc.MipLevels = 1;
			textureDesc.ArraySize = 1;
			textureDesc.SampleDesc.Count = samples;
			textureDesc.Format = format;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;
			textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

			hr = Core::Device().CreateTexture2D(&textureDesc, NULL, &result.Texture);
			if (FAILED(hr))
			{
				CORE_ERROR("DX11: Failed to create color texture for framebuffer: {}", Win32ErrorMessage(hr));
				return D3D11RenderTarget();
			}

			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
			rtvDesc.Format = textureDesc.Format;
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			hr = Core::Device().CreateRenderTargetView(result.Texture.Get(), &rtvDesc, &result.RTV);
			if (FAILED(hr))
			{
				CORE_ERROR("DX11: Failed to create render target view for framebuffer: {}", Win32ErrorMessage(hr));
				return D3D11RenderTarget();
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;

			hr = Core::Device().CreateShaderResourceView(result.Texture.Get(), &srvDesc, &result.SRV);
			if (FAILED(hr))
			{
				CORE_ERROR("DX11: Failed to create render target view for framebuffer: {}", Win32ErrorMessage(hr));
				return D3D11RenderTarget();
			}

			return result;
		}

		D3D11DepthTarget CreateDepthTarget(u32 width, u32 height, i32 samples)
		{
			D3D11DepthTarget result;
			HRESULT hr = S_OK;
			DXGI_FORMAT format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			format = IsFormatSupported(format, D3D11_BIND_DEPTH_STENCIL) ? format : c_DefaultDepth;
			D3D11_TEXTURE2D_DESC textureDesc{};
			textureDesc.Width = width;
			textureDesc.Height = height;
			textureDesc.MipLevels = 1;
			textureDesc.ArraySize = 1;
			textureDesc.SampleDesc.Count = samples;
			textureDesc.Format = format;
			textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;

			hr = Core::Device().CreateTexture2D(&textureDesc, NULL, &result.Texture);
			if (FAILED(hr))
			{
				CORE_ERROR("DX11: Failed to create color texture for framebuffer: {}", Win32ErrorMessage(hr));
				return D3D11DepthTarget();
			}

			D3D11_DEPTH_STENCIL_VIEW_DESC  rtvDesc{};
			rtvDesc.Format = textureDesc.Format;
			rtvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

			hr = Core::Device().CreateDepthStencilView(result.Texture.Get(), &rtvDesc, &result.RTV);
			if (FAILED(hr))
			{
				CORE_ERROR("DX11: Failed to create render target view for framebuffer: {}", Win32ErrorMessage(hr));
				return D3D11DepthTarget();
			}

			return result;
		}
	}
	D3D11Framebuffer::D3D11Framebuffer(const FramebufferSpecification& spec)
		: Framebuffer(spec)
	{
		for (const auto& attachment:  spec.Attachments.Attachments)
		{
			if (!IsDepthFormat(attachment.TextureFormat))
			{
				m_ColorAttachments.emplace_back(attachment);
			}
			else
			{
				m_Depthbuffer = attachment.TextureFormat;
			}
		}

		Reset();
	}
	D3D11Framebuffer::~D3D11Framebuffer()
	{
		m_ColorAttachments.clear();
		m_ColorTextures.clear();
		m_ColorSRVs.clear();
		m_ColorRTVs.clear();

		m_DepthStencilAttachment = {};

	}
	void D3D11Framebuffer::Bind()
	{
		Core::DeviceContext().OMSetRenderTargets(static_cast<u32>(m_ColorAttachments.size()), m_ColorRTVs[0].GetAddressOf(), m_DepthStencilAttachment.RTV.Get());
	}
	void D3D11Framebuffer::Unbind()
	{
		m_ColorTextures.clear();
		m_ColorSRVs.clear();
		m_ColorRTVs.clear();

		m_DepthStencilAttachment = {};
	}
	void D3D11Framebuffer::Resize(u32 width, u32 height)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;

		Reset();
	}
	i32 D3D11Framebuffer::ReadPixel(u32 attachmentIndex, i32 x, i32 y)
	{
		return i32();
	}
	void D3D11Framebuffer::ClearAttachment(u32 attachmentIndex, Color color)
	{
		CORE_ASSERT(attachmentIndex < m_ColorRTVs.size());
		const float colors[4] = { color.r, color.g, color.b, color.a };
		Core::DeviceContext().ClearRenderTargetView(m_ColorRTVs[attachmentIndex].Get(), colors);
	}
	u32 D3D11Framebuffer::GetColorAttachmentRendererID(u32 index)
	{
		return u32();
	}
	void* D3D11Framebuffer::GetImGuiAttachment(u32 id)const
	{
		return (void*)(intptr_t)m_ColorSRVs[id].Get();
	}
	void D3D11Framebuffer::Reset()
	{
		m_ColorTextures.clear();
		m_ColorSRVs.clear();
		m_ColorRTVs.clear();

		m_DepthStencilAttachment = {};

		for (const auto& attachment : m_ColorAttachments)
		{
			DXGI_FORMAT format = GetFormat(attachment.TextureFormat);
			
			
			D3D11RenderTarget renderTarget = CreateColorRenderTarget(m_Specification.Width, m_Specification.Height, format, m_Specification.Samples);
			m_ColorTextures.emplace_back(renderTarget.Texture);
			m_ColorRTVs.emplace_back(renderTarget.RTV);
			m_ColorSRVs.emplace_back(renderTarget.SRV);
		}
		if (m_Depthbuffer.TextureFormat != FramebufferTextureFormat::NONE)
		{
			m_DepthStencilAttachment = CreateDepthTarget(m_Specification.Width, m_Specification.Height, m_Specification.Samples);
		}
	}
}