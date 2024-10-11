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
		{
			ImGui::Begin(m_Name.c_str(), &m_Open);
			ImGui::Text("Application name: %s", Zephyr::Application::Get().Specification().Name.c_str());
			if (ImGui::CollapsingHeader("Renderer"))
			{
				Zephyr::RenderDevice device = Zephyr::Renderer::GetRenderDevice();
				m_vramUsage.emplace_back(device.UsedVRAM);
				if (m_vramUsage.size() > 100)
				{
					m_vramUsage.erase(m_vramUsage.begin());
				}


				ImGui::Text("Graphics API: %s", Zephyr::GetGraphicsName(Zephyr::Application::Get().Specification().WindowData.API).data());
				ImGui::Text("Rendering path: ");
				ImGui::Separator();
				ImGui::Text("Device: %s", device.Name.data());
				ImGui::Text("Vendor: %s", device.Vendor.data());
				ImGui::Text("Application used VRAM: %i/%i (MB)", device.UsedVRAM, device.AvailableVRAM);
				ImGui::PlotLines("##vram", &m_vramUsage[0], (int)m_vramUsage.size(), 0, "VRAM Usage", 0.0f, 1000.0f, ImVec2(200, 100));

				ImGui::Text("Total VRAM: %i (MB)", device.TotalVRAM);


				
			}
			
			ImGui::End();
		}
	}
}
