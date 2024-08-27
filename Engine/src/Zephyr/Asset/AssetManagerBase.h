#pragma once
#include <Zephyr/Asset/Asset.h>

namespace Zephyr
{
	using AssetMap = std::map<AssetHandle, Ref<Asset>>;

	class AssetManagerBase
	{
	public:
		virtual Ref<Asset> GetAsset(AssetHandle handle) const = 0;
		virtual bool IsAssetHandleValid(AssetHandle handle) const = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) const = 0;

		virtual AssetType GetAssetType(AssetHandle handle) const = 0;
	};
}