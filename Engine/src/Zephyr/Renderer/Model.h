#pragma once

namespace Zephyr
{
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

		void AddVertex(const Vertex& vertex)
		{
			m_Vertices.emplace_back(vertex);
		}

		// TODO: When we have materials
		void SetMaterial()
		{

		}

		size GetVertexCount() const { return m_Vertices.size(); }

	private:
		std::vector<Vertex> m_Vertices;

		friend class Model;
	};


	class Model
	{
	public:


	};
}