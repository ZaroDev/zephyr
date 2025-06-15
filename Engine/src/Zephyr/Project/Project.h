/*
MIT License

Copyright (c) 2025 ZaroDev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
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