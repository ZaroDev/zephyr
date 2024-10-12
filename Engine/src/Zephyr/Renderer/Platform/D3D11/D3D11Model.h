#pragma once

#include "D3D11Common.h"
#include <Zephyr/Renderer/Model.h>
namespace Zephyr::D3D11
{
	
	

	class D3D11Model final : public Model
	{
	public:
		D3D11Model(const std::array<std::vector<Mesh>, c_MaxLODCount>& meshes, u32 lodCount);
		~D3D11Model() = default;

		void Draw(u32 lod) override;
	private:
		std::vector<ComPtr<ID3D11Buffer>> m_VertexBuffers;
		std::array<std::vector<ComPtr<ID3D11Buffer>>, c_MaxLODCount> m_IndexBuffers;

		
	};

}