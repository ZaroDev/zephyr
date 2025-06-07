#pragma once

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
	


		static Ref<Project> New();
		static Ref<Project> Load(const Path& path);
		static bool Save(const Path& path);

	private:
		ProjectData m_Data;
		Path m_ProjectPath;

		inline static Ref<Project> s_ActiveProject;
	};
}