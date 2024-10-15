#include "ScenePanel.h"


#include <Zephyr/Renderer/Renderer.h>
#include <imgui.h>
namespace Editor
{
	ImVec2 operator-(ImVec2 v1, ImVec2 v2)
	{
		return ImVec2{ v1.x - v2.x, v1.y - v2.y };
	}
	ImVec2 operator*(ImVec2 v, float fl)
	{
		return ImVec2{ v.x * fl, v.y * fl };
	}

	ImVec2 operator+(ImVec2 v1, ImVec2 v2)
	{
		return ImVec2{ v1.x + v2.x, v1.y + v2.y };
	}

	ScenePanel::ScenePanel()
		: Panel(ICON_FK_TELEVISION "Scene", PanelCategory::WINDOW) 
	{
		m_ViewPort = Zephyr::Renderer::GetMainBuffer();
		m_LastViewportSize = { 0, 0};
	}
	ScenePanel::~ScenePanel()
	{
	}
	void ScenePanel::OnUpdate()
	{
	}
	void ScenePanel::OnImGui()
	{
		ImGui::Begin(m_Name.c_str(), &m_Open);

		if (!m_ViewPort)
		{
			ImGui::Text("No viewport available");
			ImGui::End();

			return;
		}

		ImVec2 viewport = ImGui::GetWindowSize();
		Zephyr::Iv2 view = { viewport.x, viewport.y };
		viewport.x -= 70;
		viewport.y -= 70;

		view.x = std::max(view.x, 100);
		view.y = std::max(view.y, 100);
		
		if (m_LastViewportSize != view)
		{
			m_LastViewportSize = view;
			m_ViewPort->Resize(view.x, view.y);
		}

		m_ViewPort->ClearAttachment(0, { 1.0f, 1.0f, 1.0f, 1.0f });
		ImGui::SetCursorPos((ImGui::GetContentRegionAvail() - viewport) * 0.5f + ImGui::GetWindowSize() - ImGui::GetContentRegionAvail());
		ImGui::Image(m_ViewPort->GetImGuiAttachment(0), viewport);


		ImGui::End();
	}
}