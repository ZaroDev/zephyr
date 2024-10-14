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
		D3D11RenderTarget CreateColorRenderTarget(u32 width, u32 height, DXGI_FORMAT format, u32 samples)
		{
			D3D11RenderTarget result;
			HRESULT hr = S_OK;

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

			D3D11_TEXTURE2D_DESC textureDesc{};
			textureDesc.Width = width;
			textureDesc.Height = height;
			textureDesc.MipLevels = 1;
			textureDesc.ArraySize = 1;
			textureDesc.SampleDesc.Count = samples;
			textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
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
		for (u32 i = 0; i < spec.Attachments.Attachments.size(); i++)
		{
			const auto& attachment = spec.Attachments.Attachments[i];
			if (IsDepthFormat(attachment.TextureFormat))
			{
				DXGI_FORMAT format = DXGI_FORMAT_R32G32B32_FLOAT;
				switch (attachment.TextureFormat)
				{
				case FramebufferTextureFormat::RGBA8: format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
				case FramebufferTextureFormat::RGBA32: format = DXGI_FORMAT_R32G32B32_FLOAT; break;
				case FramebufferTextureFormat::RED_INTEGER: format = DXGI_FORMAT_R8_UINT; break;
				}
				D3D11RenderTarget renderTarget = CreateColorRenderTarget(spec.Width, spec.Height, format, spec.Samples);
				m_ColorTextures.emplace_back(renderTarget.Texture);
				m_ColorRTVs.emplace_back(renderTarget.RTV);
				m_ColorSRVs.emplace_back(renderTarget.SRV);
				m_ColorAttachments.emplace_back(attachment);
			}
			else
			{
				m_Depthbuffer = attachment.TextureFormat;
				m_DepthStencilAttachment = CreateDepthTarget(spec.Width, spec.Height, spec.Samples);
			}
		}
	}
	D3D11Framebuffer::~D3D11Framebuffer()
	{
	}
	void D3D11Framebuffer::Bind()
	{
		Core::DeviceContext().OMSetRenderTargets(m_ColorAttachments.size(), m_ColorRTVs[0].GetAddressOf(), m_DepthStencilAttachment.RTV.Get());
	}
	void D3D11Framebuffer::Unbind()
	{
	}
	void D3D11Framebuffer::Resize(u32 width, u32 height)
	{
	}
	i32 D3D11Framebuffer::ReadPixel(u32 attachmentIndex, i32 x, i32 y)
	{
		return i32();
	}
	void D3D11Framebuffer::ClearAttachment(u32 attachmentIndex, i32 value)
	{

	}
	u32 D3D11Framebuffer::GetColorAttachmentRendererID(u32 index)
	{
		return u32();
	}
}