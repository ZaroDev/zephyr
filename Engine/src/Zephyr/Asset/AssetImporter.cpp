#include <pch.h>
#include "AssetImporter.h"
#include "TextureImporter.h"

namespace Zephyr
{
	using AssetImportFunction = std::function<Ref<Asset>(AssetHandle, const AssetMetadata&)>;
	static std::map<AssetType, AssetImportFunction> s_AssetImportFuncs = {

		{AssetType::TEXTURE2D, TextureImporter::ImportTexture2D },
	};


	Ref<Asset> AssetImporter::ImportAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		if (!s_AssetImportFuncs.contains(metadata.Type))
		{
			CORE_ERROR("No importer available for asset type: {}", metadata.Type);
			return nullptr;
		}

		return s_AssetImportFuncs.at(metadata.Type)(handle, metadata);
	}
}