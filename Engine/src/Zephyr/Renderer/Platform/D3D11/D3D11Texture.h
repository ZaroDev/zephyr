#pragma once
#ifdef PLATFORM_WINDOWS
#include "D3D11Common.h"
#include <Zephyr/Renderer/Texture.h>


namespace Zephyr::D3D11
{
	class D3D11Texture2D final : public Texture2D
	{
	public:
		D3D11Texture2D(TextureSpecification spec, Buffer buffer = Buffer());
		virtual ~D3D11Texture2D();

		virtual const TextureSpecification& GetSpecification() const override { return m_Specification; }

		virtual u32 GetWidth() const override { return m_Width; }
		virtual u32 GetHeight() const override { return m_Height; }
		virtual u32 GetRendererID() const override { return m_RendererID; }

		virtual void SetData(void* data, u32 size) override {}

		virtual void Bind(u32 slot = 0) const override;

		virtual bool IsLoaded() const override { return m_Texture; }

		virtual bool operator==(const Texture& other) const override 
		{
			return false;
		}

	private:
		TextureSpecification m_Specification;
		bool m_IsLoaded = false;

		u32 m_Width, m_Height;
		u32 m_RendererID;

		ComPtr<ID3D11SamplerState> m_SamplerState = nullptr;
		ComPtr<ID3D11ShaderResourceView> m_Texture = nullptr;
	};
}

#endif