#pragma once
#include <Zephyr/Asset/Asset.h>
#include <Zephyr/Asset/AssetMetadata.h>

namespace Zephyr
{
	class AssetImporter final
	{
	public:
		static Ref<Asset> ImportAsset(AssetHandle handle, const AssetMetadata& metadata);
	};
}