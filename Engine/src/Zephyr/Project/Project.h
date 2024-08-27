#pragma once

#include <Zephyr/Asset/AssetManagerBase.h>
#include <Zephyr/Asset/EditorAssetManager.h>
#include <Zephyr/Asset/RuntimeAssetManager.h>

namespace Zephyr
{
	struct ProjectData
	{
		std::string Name = "Untitled";

		Path StartScene;
		
		Path WorkingDirectory;
		Path AssetRegistryPath;
	};


	class Project
	{
	public:
		const Path& GetProjectDirectory() { return m_ProjectPath; }


		static const Path& GetProjectPath()
		{
			CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->m_ProjectPath;
		}

		static Path GetWorkingDirectory()
		{
			CORE_ASSERT(s_ActiveProject);
			return GetProjectPath() / s_ActiveProject->m_Data.WorkingDirectory;
		}

		static Path GetAssetRegistryPath()
		{
			CORE_ASSERT(s_ActiveProject);
			return GetProjectPath() / s_ActiveProject->m_Data.AssetRegistryPath;
		}

		ProjectData& GetData() { return m_Data;  }

		static Ref<Project> GetActive() { return s_ActiveProject; }
		Ref<AssetManagerBase> GetAssetManager() { return m_AssetManager; }
		Ref<EditorAssetManager> GetEditorAssetManager() { return Cast<EditorAssetManager>(m_AssetManager); }
		Ref<RuntimeAssetManager> GetRuntimeAssetManager() { return Cast<RuntimeAssetManager>(m_AssetManager); }


		static Ref<Project> New();
		static Ref<Project> Load(const Path& path);
		static bool Save(const Path& path);

	private:
		ProjectData m_Data;
		Path m_ProjectPath;

		Ref<AssetManagerBase> m_AssetManager;

		inline static Ref<Project> s_ActiveProject;
	};
}