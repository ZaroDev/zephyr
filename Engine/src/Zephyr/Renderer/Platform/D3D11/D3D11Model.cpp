#include <pch.h>
#include "D3D11Model.h"

#include "D3D11Renderer.h"

namespace Zephyr::D3D11
{
	D3D11Model::D3D11Model(const std::array<std::vector<Mesh>, c_MaxLODCount>& lodMeshes, u32 lodCount)
		: Model(lodMeshes,lodCount)
	{
		const std::vector<Mesh>& meshes = lodMeshes[0];
		m_VertexBuffers.resize(meshes.size());
		for (u32 i = 0; i < meshes.size(); i++)
		{
			const auto& mesh = meshes[i];
			D3D11_BUFFER_DESC bufferInfo = {};
			bufferInfo.ByteWidth = mesh.GetVertexCount() * sizeof(Vertex);
			bufferInfo.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
			bufferInfo.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA resourceData = {};
			resourceData.pSysMem = mesh.GetVertices().data();

			HRESULT hr = Core::Device().CreateBuffer(&bufferInfo, &resourceData, &m_VertexBuffers[i]);
			if (FAILED(hr))
			{
				CORE_ERROR("D3D11: Failed to create triangle vertex buffer: {0}", Win32ErrorMessage(hr));
			}
		}
		for (u32 lod = 0; lod < c_MaxLODCount; lod++)
		{
			const std::vector<Mesh>& meshes = lodMeshes[lod];
			m_IndexBuffers[lod].resize(meshes.size());
			for (u32 i = 0; i < meshes.size(); i++)
			{

				const auto& mesh = meshes[i];
				D3D11_BUFFER_DESC bufferInfo = {};
				bufferInfo.ByteWidth = mesh.GetIndexCount() * sizeof(u32);
				bufferInfo.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
				bufferInfo.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER;

				D3D11_SUBRESOURCE_DATA resourceData = {};
				resourceData.pSysMem = mesh.GetIndices().data();

				HRESULT hr = Core::Device().CreateBuffer(&bufferInfo, &resourceData, &m_IndexBuffers[lod][i]);
				if (FAILED(hr))
				{
					CORE_ERROR("D3D11: Failed to create index buffer for LOD {0}: {1}", lod, Win32ErrorMessage(hr));
				}

			}
		}
		// TODO: Move this to a geometry pass

		
	}
	void D3D11Model::Draw(u32 lod)
	{
		auto& deviceContext = Core::DeviceContext();
		UINT vertexStride = sizeof(Vertex);
		UINT vertexOffset = 0;
		
		deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		for (u32 i = 0; i < m_VertexBuffers.size(); i++)
		{
			const auto& vertexBuffer = m_VertexBuffers[i];
			const auto& indexBuffer = m_IndexBuffers[lod][i];
			const u32 indexCount = m_Meshes[lod][i].GetIndexCount();

			deviceContext.IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);
			deviceContext.IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
			

			deviceContext.DrawIndexed(indexCount, 0, 0);
		}
	}
}