#include <pch.h>
#include "D3D11FullScreenPass.h"

#include "D3D11Renderer.h"

namespace Zephyr::D3D11
{
	D3D11FullScreenPass::D3D11FullScreenPass(Ref<D3D11Shader> shader)
		: m_Shader(shader)
	{
		{
			constexpr D3D11_INPUT_ELEMENT_DESC vertexInputLayoutInfo[] =
			{
				{
					"POSITION",
					0,
					DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT,
					0,
					offsetof(FullscreenPassLayout, Position),
					D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
					0
				},
				{
					"TEXCOORD",
					0,
					DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,
					0,
					offsetof(FullscreenPassLayout, TexCoords),
					D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA,
					0
				}
			};



			if (FAILED(Core::Device().CreateInputLayout(vertexInputLayoutInfo, _countof(vertexInputLayoutInfo), shader->GetVertexBlob()->GetBufferPointer(), shader->GetVertexBlob()->GetBufferSize(),
				&m_InputLayout)))
			{
				CORE_ERROR("D3D11: Failed to create vertex input layout!");
			}
		}
		{
			constexpr float quadVertices[] = {
				// positions   // texCoords
				-1.0f,  1.0f,  0.0f, 1.0f,
				-1.0f, -1.0f,  0.0f, 0.0f,
				1.0f, -1.0f,  1.0f, 0.0f,

				-1.0f,  1.0f,  0.0f, 1.0f,
				1.0f, -1.0f,  1.0f, 0.0f,
				1.0f,  1.0f,  1.0f, 1.0f
			};


			D3D11_BUFFER_DESC bufferInfo = {};
			bufferInfo.ByteWidth = _countof(quadVertices) * sizeof(float);
			bufferInfo.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
			bufferInfo.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA resourceData = {};
			resourceData.pSysMem = quadVertices;

			HRESULT hr = Core::Device().CreateBuffer(&bufferInfo, &resourceData, &m_QuadBuffer);
			if (FAILED(hr))
			{
				CORE_ERROR("D3D11: Failed to create triangle vertex buffer: {0}", Win32ErrorMessage(hr));
			}
		}
	}
	void D3D11FullScreenPass::Render()
	{
		UINT vertexStride = sizeof(FullscreenPassLayout);
		UINT vertexOffset = 0;

		auto& context = Core::DeviceContext();

		context.IASetInputLayout(m_InputLayout.Get());
		context.VSSetShader(m_Shader->GetVertex().Get(), nullptr, 0);
		context.PSSetShader(m_Shader->GetPixel().Get(), nullptr, 0);
		context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.IASetVertexBuffers(0, 1, m_QuadBuffer.GetAddressOf(), &vertexStride, &vertexOffset);
		context.Draw(4, 0);

	}
}
