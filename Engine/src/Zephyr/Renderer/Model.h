#pragma once
#include <Zephyr/Asset/Asset.h>
#include <Zephyr/FileSystem/Buffer.h>

namespace Zephyr
{
	constexpr u32 c_MaxLODCount = 4;
	struct Vertex
	{
		V3 Position;
		V3 Normal;
		V2 TexCoord;
	};

	// Implementation of what a mesh should contain
	// A set of vertices and a material reference
	class Mesh
	{
	public:
		Mesh() = default;
		virtual ~Mesh() = default;

		DEFAULT_MOVE_AND_COPY(Mesh);

		Mesh(const std::vector<Vertex>& vertices, const std::vector<u32> indices,AssetHandle materialHandle)
			: m_Vertices(std::move(vertices)), m_Indices(std::move(indices)), m_MaterialHandle(materialHandle)
		{
		}

		bool IsValid() const
		{
			return !m_Vertices.empty() && !m_Indices.empty();
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
		size GetIndexCount() const { return m_Indices.size(); }

		const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
		const std::vector<u32>& GetIndices() const { return m_Indices; }
	private:
		std::vector<Vertex> m_Vertices;
		std::vector<u32> m_Indices;
		AssetHandle m_MaterialHandle;
		friend class Model;
	};


	class Model : public Asset
	{
	public:
		Model(std::array<std::vector<Mesh>, c_MaxLODCount> meshes,u32 lodCount)
			: m_Meshes(meshes), m_LODCount(lodCount)
		{}
		
		static AssetType GetStaticType() { return AssetType::MODEL; }
		virtual AssetType GetType() const override { return GetStaticType(); }

		size GetMeshCount() const { return m_Meshes.size(); }

		static Ref<Model> Create(const std::array<std::vector<Mesh>, c_MaxLODCount>& meshes, u32 lodCount);

		const std::vector<Mesh>& GetMeshesAtLOD(u32 lod) const { return m_Meshes[lod]; }

		u32 GetLODCount() const { return m_LODCount; };
	protected:
		std::array<std::vector<Mesh>, c_MaxLODCount> m_Meshes;
		u32 m_LODCount;
	};
}