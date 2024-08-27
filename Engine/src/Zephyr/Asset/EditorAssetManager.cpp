#include <pch.h>
#include "EditorAssetManager.h"
#include <Zephyr/Asset/AssetImporter.h>
#include <Zephyr/Project/Project.h>

namespace Zephyr
{
    Ref<Asset> EditorAssetManager::GetAsset(AssetHandle handle) const
    {
        if (!IsAssetHandleValid(handle))
        {
            return nullptr;
        }

        if (IsAssetLoaded(handle)) 
        {
            return m_LoadedAssets.at(handle);
        }

        const AssetMetadata& metadata = GetMetadata(handle);
        Ref<Asset> asset = AssetImporter::ImportAsset(handle, metadata);
        if (!asset)
        {
            // Import failed
            CORE_ERROR("EditorAssetManager: Importing asset {0} failed", metadata.FilePath);
        }

        return asset;
    }
    bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle) const
    {
        return handle != 0 && m_AssetRegistry.contains(handle);
    }
    bool EditorAssetManager::IsAssetLoaded(AssetHandle handle) const
    {
        return m_LoadedAssets.contains(handle);
    }
    const AssetMetadata& EditorAssetManager::GetMetadata(AssetHandle handle) const
    {
        static AssetMetadata s_NullMetadata;
        auto it = m_AssetRegistry.find(handle);
        if (it == m_AssetRegistry.end())
        {
            return s_NullMetadata;
        }

        return it->second;
    }
    const Path& EditorAssetManager::GetFilePath(AssetHandle handle) const
    {
        return GetMetadata(handle).FilePath;
    }
    void EditorAssetManager::SerializeAssetRegistry()
    {
       
    }
    bool EditorAssetManager::DeserializeAssetRegistry()
    {
        return false;
    }
}