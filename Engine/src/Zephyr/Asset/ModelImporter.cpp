#include <pch.h>
#include "ModelImporter.h"
#include <Zephyr/Project/Project.h>
#include <Zephyr/FileSystem/Buffer.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <meshoptimizer.h>

#include <glm/gtc/type_ptr.hpp>

namespace Zephyr
{
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, u32 lod)
	{
		std::vector<Vertex> vertices;
		std::vector<u32> indices;

		for (u32 i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vert;
			vert.Position.x = mesh->mVertices[i].x;
			vert.Position.y = mesh->mVertices[i].y;
			vert.Position.z = mesh->mVertices[i].z;

			vert.Normal.x = mesh->mNormals[i].x;
			vert.Normal.y = mesh->mNormals[i].y;
			vert.Normal.z = mesh->mNormals[i].z;

			if (mesh->mTextureCoords[0]) 
			{
				vert.TexCoord.x = mesh->mTextureCoords[0][i].x;
				vert.TexCoord.y = mesh->mTextureCoords[0][i].y;

			}
			else
			{
				vert.TexCoord = V2(0.0f, 0.0f);
			}
			vertices.emplace_back(vert);
		}
		for (u32 i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (u32 j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j]);
			}
		}
		// LOD 0 has no optimizations
		

		// Optimize the mesh
		std::vector<u32> remap(indices.size());
		const size vertexCount = meshopt_generateVertexRemap(remap.data(), 
			indices.data(), indices.size(), 
			vertices.data(), indices.size(), sizeof(Vertex));

		std::vector<u32> remappedIndices(indices.size());
		std::vector<Vertex> remappedVertices(vertexCount);

		meshopt_remapIndexBuffer(remappedIndices.data(), indices.data(), indices.size(), remap.data());
		meshopt_remapVertexBuffer(remappedVertices.data(), vertices.data(), vertices.size(), sizeof(Vertex), remap.data());

		meshopt_optimizeVertexCache(remappedIndices.data(), remappedIndices.data(), indices.size(), vertexCount);
		meshopt_optimizeOverdraw(remappedIndices.data(), remappedIndices.data(), indices.size(),
		 glm::value_ptr(remappedVertices[0].Position), vertexCount, sizeof(Vertex), 1.05f);

		meshopt_optimizeVertexFetch(remappedVertices.data(),
			remappedIndices.data(),
			indices.size(),
			remappedVertices.data(),
			vertexCount,
			sizeof(Vertex));

		if (!lod)
		{
			return Mesh(remappedVertices, remappedIndices, AssetHandle());
		}

		float threshold = 0.f;
		switch (lod)
		{
		case 1: threshold = 0.2f; break;
		case 2: threshold = 0.1f; break;
		case 3: threshold = 0.05f;  break;
		default: threshold = 0.2f; break;
		}

		const size targetIndexCount = size(remappedIndices.size() * threshold);
		const float targetError = 1e-2f;

		std::vector<u32> indicesLod(remappedIndices.size());
		const size lodSize = meshopt_simplify(&indicesLod[0], remappedIndices.data(), remappedIndices.size(), 
			&remappedVertices[0].Position.x, vertexCount, sizeof(Vertex), targetIndexCount, targetError);
		indicesLod.resize(lodSize);

		return Mesh(remappedVertices, indicesLod, AssetHandle());
	}

	void ProcessNode(aiNode* node, const aiScene* scene, std::array<std::vector<Mesh>, c_MaxLODCount>& meshes)
	{
		// process all the node's meshes (if any)
		for (u32 i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			for (u32 lod = 0; lod < c_MaxLODCount; lod++)
			{
				Mesh lodMesh = ProcessMesh(mesh, scene, lod);
				meshes[lod].emplace_back(lodMesh);
			}
		}
		for (u32 i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, meshes);
		}
	}

	Ref<Model> ModelImporter::ImportModel(AssetHandle handle, const AssetMetadata& metadata)
	{
		return LoadModel(Project::GetWorkingDirectory() / metadata.FilePath);
	}
	Ref<Model> ModelImporter::LoadModel(const Path& path)
	{
		Assimp::Importer importer;

		const int importFlags = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace;
		const aiScene* scene = importer.ReadFile(path.string().c_str(), importFlags);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			CORE_ERROR("Model importer Assimp error {0}", importer.GetErrorString());
			return nullptr;
		}
		std::array<std::vector<Mesh>, c_MaxLODCount> meshes;
		ProcessNode(scene->mRootNode, scene, meshes);
		return Model::Create(meshes,4);
	}
}