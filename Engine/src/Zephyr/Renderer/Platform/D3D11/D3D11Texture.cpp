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
		Core::CreateTexture(*this, buffer);
	}
	D3D11Texture2D::~D3D11Texture2D()
	{
		Texture.Reset();
		m_SamplerState.Reset();
	}
	void D3D11Texture2D::Bind(u32 slot) const
	{
		Core::DeviceContext().PSSetShaderResources(slot, 1, Texture.GetAddressOf());
		Core::DeviceContext().PSSetSamplers(slot, 1, m_SamplerState.GetAddressOf());
	}
}

#endif