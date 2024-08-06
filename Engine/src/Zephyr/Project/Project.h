#pragma once

namespace Zephyr
{
	struct ProjectData
	{
		std::string Name = "Untitled";

		Path StartScene;
		Path WorkingDirectory;
	};


	class Project
	{
	public:

		static const Path& Directory()
		{
			CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->m_ProjectPath;
		}

		static Path WorkingDirectory()
		{
			CORE_ASSERT(s_ActiveProject);
			return Directory() / s_ActiveProject->m_Data.WorkingDirectory;
		}

		ProjectData& Data() { return m_Data;  }

		static Ref<Project> Get() { return s_ActiveProject; }

		static Ref<Project> New();
		static Ref<Project> Load(const Path& path);
		static bool Save(const Path& path);

	private:
		ProjectData m_Data;
		Path m_ProjectPath;

		inline static Ref<Project> s_ActiveProject;
	};
}