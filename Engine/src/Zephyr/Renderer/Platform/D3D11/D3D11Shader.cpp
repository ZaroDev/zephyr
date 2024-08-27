#include <pch.h>
#include "D3D11Shader.h"
#include "D3D11Renderer.h"
#include <d3dcompiler.h>

#include <Zephyr/Core/Application.h>

namespace Zephyr::D3D11
{
	D3D11Shader::D3D11Shader(const Path& filePath)
	{
		m_Name = filePath.string();
		Path file = c_ShaderPath;
		file /= filePath;

		m_VertexShader = CreateVertexShader(FileSystem::ReplaceExtension(file, "vs.hlsl"), m_VertexShaderBlob);
		m_PixelShader = CreatePixelShader(FileSystem::ReplaceExtension(file, "ps.hlsl"), m_PixelShaderBlob);

		CreateInputLayout();
	}
	void D3D11Shader::Bind()
	{
	}
	void D3D11Shader::UnBind()
	{
	}
	void D3D11Shader::CreateInputLayout()
	{
		constexpr D3D11_INPUT_ELEMENT_DESC vertexInputLayoutInfo[] =
		{
			{
				"POSITION",
				0,
				DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				0,
				offsetof(Core::VertexPositionColor, Position),
				D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				0
			},
			{
				"COLOR",
				0,
				DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
				0,
				offsetof(Core::VertexPositionColor, Color),
				D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
				0
			},
		};

		

		if (FAILED(Core::Device().CreateInputLayout(vertexInputLayoutInfo, _countof(vertexInputLayoutInfo), m_VertexShaderBlob->GetBufferPointer(), m_VertexShaderBlob->GetBufferSize(),
			&m_VertexLayout)))
		{
			CORE_ERROR("D3D11: Failed to create vertex input layout!");

		}
	}
	bool D3D11Shader::CompileShader(const Path& filePath, std::string_view profile, ComPtr<ID3DBlob>& shaderBlob) const
	{
		constexpr UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

		ComPtr<ID3DBlob> tempShaderBlob = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;

		if (FAILED(D3DCompileFromFile(filePath.wstring().data(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, c_EntryPoint.data(), profile.data(), compileFlags,
			0, &tempShaderBlob, &errorBlob)))
		{
			CORE_ERROR("D3D11: Failed to read shader from file");
			if (errorBlob != nullptr)
			{
				CORE_ERROR("D3D11: Shader error {0}", static_cast<const char*>(errorBlob->GetBufferPointer()));
			}

			return false;
		}

		shaderBlob = std::move(tempShaderBlob);

		return true;
	}
	ComPtr<ID3D11VertexShader> D3D11Shader::CreateVertexShader(const Path& filePath, ComPtr<ID3DBlob>& vertexShaderBlob)
	{
		if (!CompileShader(filePath, c_ProfileVersion[ShaderType::VERTEX], vertexShaderBlob))
		{
			return nullptr;
		}

		ComPtr<ID3D11VertexShader> vertexShader;
		if (FAILED(Core::Device().CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertexShader)))
		{
			CORE_ERROR("D3D11: Failed to compile vertex shader");
			return nullptr;
		}

		return vertexShader;
	}
	ComPtr<ID3D11PixelShader> D3D11Shader::CreatePixelShader(const std::filesystem::path& filePath, ComPtr<ID3DBlob>& pixelShaderBlob)
	{
		if (!CompileShader(filePath, c_ProfileVersion[ShaderType::FRAGMENT], pixelShaderBlob))
		{
			return nullptr;
		}

		ComPtr<ID3D11PixelShader> pixelShader;
		if (FAILED(Core::Device().CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &pixelShader)))
		{
			CORE_ERROR("D3D11: Failed to compile vertex shader");
			return nullptr;
		}

		return pixelShader;
	}
}