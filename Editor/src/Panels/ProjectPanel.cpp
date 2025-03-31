#include "ProjectPanel.h"
#include <imgui.h>

#include "Zephyr/Core/Log.h"


namespace Editor
{
	void ProjectPanel::OnUpdate()
	{
	}
	void ProjectPanel::OnImGui()
	{
		ImGui::Begin(m_Name.c_str(), &m_Open);

		ImGui::End();
	}
}
