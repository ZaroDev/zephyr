#pragma once
#include <Zephyr/Asset/Asset.h>
#include <Zephyr/FileSystem/Buffer.h>

namespace Zephyr
{
	constexpr u32 c_MaxLODCount = 4;
	struct Vertex
	{
		V3 Position;
		V2 TexCoord;
		V3 Normal;
	};

	// Implementation of what a mesh should contain
	// A set of vertices and a material reference
	class Mesh final
	{
	public:
		Mesh() = default;
		~Mesh() = default;

		DEFAULT_MOVE_AND_COPY(Mesh);

		Mesh(const std::vector<Vertex> vertices, AssetHandle materialHandle)
		{

		}

		void AddVertex(const Vertex& vertex)
		{
			m_Vertices.emplace_back(vertex);
		}

		// TODO: When we have materials
		void SetMaterial(AssetHandle material)
		{
			m_MaterialHandle = material;
		}

		size GetVertexCount() const { return m_Vertices.size(); }

	private:
		std::vector<Vertex> m_Vertices;
		AssetHandle m_MaterialHandle;
		friend class Model;
	};


	class Model final : public Asset
	{
	public:

		
		static AssetType GetStaticType() { return AssetType::MODEL; }
		virtual AssetType GetType() const override { return GetStaticType(); }

		size GetMeshCount() const { return m_Meshes.size(); }
	private:
		std::array<std::vector<Mesh>, c_MaxLODCount> m_Meshes;
		u32 m_LODCount;
	};
}