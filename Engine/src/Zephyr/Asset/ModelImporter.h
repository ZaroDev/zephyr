
#pragma once
#include <Zephyr/Asset/Asset.h>
#include <Zephyr/Asset/AssetMetadata.h>

#include <Zephyr/Renderer/Model.h>

namespace Zephyr
{
	class ModelImporter
	{
	public:
		// AssetMetadata filepath is relative to project asset directory
		static Ref<Model> ImportModel(AssetHandle handle, const AssetMetadata& metadata);

		// Reads file directly from filesystem
		// (i.e. path has to be relative / absolute to working directory)
		static Ref<Model> LoadModel(const Path& path);
	};
}