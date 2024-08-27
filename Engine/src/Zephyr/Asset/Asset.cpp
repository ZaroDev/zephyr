#include "pch.h"
#include "Asset.h"


namespace Zephyr
{
	std::string_view AssetTypeToString(AssetType type)
	{
		switch (type)
		{
		case AssetType::NONE: return "AssetType::NONE";
		case AssetType::SCENE: return "AssetType::SCENE";
		case AssetType::TEXTURE2D: return "AssetType::TEXTURE2D";
		case AssetType::MESH: return "AssetType::MESH";
		}

		return "AssetType::<Invalid>";
	}

	AssetType AssetTypeFromString(std::string_view assetType)
	{
		if (assetType == "AssetType::SCENE") return AssetType::SCENE;
		if (assetType == "AssetType::TEXTURE2D") return AssetType::TEXTURE2D;
		if (assetType == "AssetType::MESH") return AssetType::MESH;


		return AssetType::NONE;
	}

}