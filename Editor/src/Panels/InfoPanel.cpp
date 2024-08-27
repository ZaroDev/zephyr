#include "InfoPanel.h"

#include <imgui.h>
#include <Zephyr/Core/Base.h>
#include <Zephyr/Core/Application.h>
#include <Zephyr/Renderer/Renderer.h>


namespace Editor
{
	void InfoPanel::OnUpdate()
	{
	}
	void InfoPanel::OnImGui()
	{
		ImGui::Begin(m_Name.c_str(), &m_Open);
		ImGui::Text("Application name: %s", Zephyr::Application::Get().Specification().Name.c_str());
		ImGui::Text("Renderer: %s", Zephyr::GetGraphicsName(Zephyr::Application::Get().Specification().API).data());
		ImGui::End();
	}
}
