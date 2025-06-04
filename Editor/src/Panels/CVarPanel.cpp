#include "CVarPanel.h"
#include <Zephyr/Core/CVarManager.h>
#include <imgui.h>

namespace Editor
{
	void CVarPanel::OnUpdate()
	{
	}
	void CVarPanel::OnImGui()
	{
		ImGui::Begin(m_Name.c_str(), &m_Open);
		Zephyr::CVarManager::Get()->DrawImGuiEditor();
		ImGui::End();
	}
}