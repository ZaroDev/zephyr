#pragma once

#include <Zephyr/Asset/AssetManagerBase.h>
#include <Zephyr/Project/Project.h>

namespace Zephyr
{
	class AssetManager
	{
	public:
		template<typename T>
		static Ref<T> GetAsset(AssetHandle handle)
		{
			return Cast<T>(Project::GetActive()->GetAssetManager()->GetAsset(handle));
		}

		static bool IsAssetHandleValid(AssetHandle handle)
		{
			return Project::GetActive()->GetAssetManager()->IsAssetHandleValid(handle);
		}

		static bool IsAssetLoaded(AssetHandle handle)
		{
			return Project::GetActive()->GetAssetManager()->IsAssetLoaded(handle);
		}

		static AssetType GetAssetType(AssetHandle handle) 
		{
			return Project::GetActive()->GetAssetManager()->GetAssetType(handle);
		}
	};
}