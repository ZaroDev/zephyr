#include <pch.h>
#include "Project.h"

namespace Zephyr
{
	Ref<Project> Project::New()
	{
		s_ActiveProject = CreateRef<Project>();

		return s_ActiveProject;
	}

	Ref<Project> Project::Load(const Path& path)
	{
		return Ref<Project>();
	}

	bool Project::Save(const Path& path)
	{
		return false;
	}


}