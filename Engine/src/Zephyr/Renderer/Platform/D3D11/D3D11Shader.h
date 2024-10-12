#pragma once

#include "D3D11Common.h"

#include <Zephyr/Renderer/Shader.h>

namespace Zephyr::D3D11
{
	constexpr StrView c_ProfileVersion[ShaderType::MAX] =
	{
		"vs_5_0",
		"ps_5_0",
	};
	constexpr StrView c_EntryPoint = "Main";

	static_assert(_countof(c_ProfileVersion) == ShaderType::MAX);

	const Path c_ShaderPath = "Shaders\\D3D11";

	class D3D11Shader final : public Shader
	{
	public:
		D3D11Shader(const Path& filePath);
		void Bind() override;
		void UnBind() override;

		ComPtr<ID3D11InputLayout> GetLayout() const { return m_VertexLayout; }
		ComPtr<ID3D11VertexShader> GetVertex() const { return m_VertexShader; }
		ComPtr<ID3D11PixelShader> GetPixel() const { return m_PixelShader; }
		
	private:
		void CreateInputLayout();

		bool CompileShader(const Path& filePath, std::string_view profile, ComPtr<ID3DBlob>& shaderBlob) const;

		ComPtr<ID3D11VertexShader> CreateVertexShader(const Path& filePath, ComPtr<ID3DBlob>& vertexShaderBlob);
		ComPtr<ID3D11PixelShader> CreatePixelShader(const Path& filePath, ComPtr<ID3DBlob>& pixelShaderBlob);

	private:
		ComPtr<ID3D11InputLayout> m_VertexLayout;
		ComPtr<ID3DBlob> m_PixelShaderBlob;
		ComPtr<ID3DBlob> m_VertexShaderBlob;
		ComPtr<ID3D11VertexShader> m_VertexShader;
		ComPtr<ID3D11PixelShader> m_PixelShader;
	};
}