#pragma once
#include "D3D11Common.h"
#include "D3D11Shader.h"

namespace Zephyr::D3D11
{
	struct FullscreenPassLayout
	{
		V3 Position;
		V2 TexCoords;
	};

	class D3D11FullScreenPass final
	{
	public:
		D3D11FullScreenPass(Ref<D3D11Shader> shader);
		~D3D11FullScreenPass() = default;


		void Render();
	private:
		Ref<D3D11Shader> m_Shader = nullptr;
		ComPtr<ID3D11InputLayout> m_InputLayout = nullptr;
		ComPtr<ID3D11Buffer> m_QuadBuffer = nullptr;
	};
}