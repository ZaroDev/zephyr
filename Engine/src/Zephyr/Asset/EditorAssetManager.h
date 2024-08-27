#pragma once
#include <Zephyr/Asset/AssetMetadata.h>
#include <Zephyr/Asset/AssetManagerBase.h>


namespace Zephyr
{
	
	using AssetRegistry = std::map<AssetHandle, AssetMetadata>;

	class EditorAssetManager final : public AssetManagerBase
	{
	public:
		virtual Ref<Asset> GetAsset(AssetHandle handle) const override;

		virtual bool IsAssetHandleValid(AssetHandle handle) const override;
		virtual bool IsAssetLoaded(AssetHandle handle) const override;

		const AssetMetadata& GetMetadata(AssetHandle handle) const;
		const Path& GetFilePath(AssetHandle handle) const;

		const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }

		void SerializeAssetRegistry();
		bool DeserializeAssetRegistry();
	private:
		AssetRegistry m_AssetRegistry;
		AssetMap m_LoadedAssets;
	};
}