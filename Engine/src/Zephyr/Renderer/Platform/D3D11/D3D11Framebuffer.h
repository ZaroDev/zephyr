#pragma once

#include "D3D11Common.h"
#include <Zephyr/Renderer/Framebuffer.h>

namespace Zephyr::D3D11
{
	struct D3D11RenderTarget
	{
		ComPtr<ID3D11Texture2D> Texture = nullptr;
		ComPtr<ID3D11RenderTargetView> RTV = nullptr;
		ComPtr<ID3D11ShaderResourceView> SRV = nullptr;
	};
	struct D3D11DepthTarget
	{
		ComPtr<ID3D11Texture2D> Texture = nullptr;
		ComPtr<ID3D11DepthStencilView> RTV = nullptr;
	};

	constexpr DXGI_FORMAT c_DefaultFormat = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
	constexpr DXGI_FORMAT c_DefaultDepth = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;

	class D3D11Framebuffer final : public Framebuffer
	{
	public:
		D3D11Framebuffer(const FramebufferSpecification& spec);
		~D3D11Framebuffer() override;

		void Bind() override;
		void Unbind() override;
		
		void Resize(u32 width, u32 height) override;
		i32	ReadPixel(u32 attachmentIndex, i32 x, i32 y) override;
		void ClearAttachment(u32 attachmentIndex, i32 value) override;
		u32 GetColorAttachmentRendererID(u32 index = 0) override;
	private:
		void Reset();


		std::vector<ComPtr<ID3D11Texture2D>> m_ColorTextures;
		std::vector<ComPtr<ID3D11RenderTargetView>> m_ColorRTVs;
		std::vector<ComPtr<ID3D11ShaderResourceView>> m_ColorSRVs;
		D3D11DepthTarget m_DepthStencilAttachment{};
	};

}