#include <pch.h>

#ifdef PLATFORM_WINDOWS
#include "D3D11Texture.h"
#include <Zephyr/Core/Application.h>
#include <Zephyr/Renderer/Platform/D3D11/D3D11Renderer.h>



namespace Zephyr::D3D11
{
	D3D11Texture2D::D3D11Texture2D(TextureSpecification spec, Buffer buffer)
	{
		m_Specification = spec;
		m_Width = spec.Width;
		m_Height = spec.Height;
		D3D11_TEXTURE2D_DESC desc{};
		ZeroMemory(&desc, sizeof(desc));

		desc.Width = m_Width;
		desc.Height = m_Height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;

		switch (m_Specification.Format)
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
		imageSubresouceData.SysMemPitch = m_Width * m_Specification.Channels;

		ComPtr<ID3D11Texture2D> imageTexture;

		auto& device = Core::Device();
		if (FAILED(device.CreateTexture2D(&desc, &imageSubresouceData, &imageTexture)))
		{
			CORE_ERROR("D3D11: Failed to create texture 2d");
			return;
		}

		if (FAILED(device.CreateShaderResourceView(imageTexture.Get(), nullptr, &m_Texture)))
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

		if (FAILED(device.CreateSamplerState(&samplerDesc, &m_SamplerState)))
		{
			CORE_ERROR("D3D11: Failed to crate sampler state");
		}
	}
	D3D11Texture2D::~D3D11Texture2D()
	{
		m_Texture.Reset();
		m_SamplerState.Reset();
	}
	void D3D11Texture2D::Bind(u32 slot) const
	{
		Core::DeviceContext().PSSetShaderResources(slot, 1, m_Texture.GetAddressOf());
		Core::DeviceContext().PSSetSamplers(slot, 1, m_SamplerState.GetAddressOf());
	}
}

#endif