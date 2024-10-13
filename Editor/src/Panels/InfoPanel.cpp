#include "InfoPanel.h"

#include <imgui.h>
#include <Zephyr/Core/Base.h>
#include <Zephyr/Core/Application.h>
#include <Zephyr/Renderer/Renderer.h>

#include <Zephyr/Time/Time.h>

namespace Editor
{
	void InfoPanel::OnUpdate()
	{
		m_FPSRecord.emplace_back(Zephyr::Time::GetFPS());
		if (m_FPSRecord.size() > 100)
		{
			m_FPSRecord.erase(m_FPSRecord.begin());
		}

		m_MSRecord.emplace_back(Zephyr::Time::GetDeltaTime());
		if (m_MSRecord.size() > 100)
		{
			m_MSRecord.erase(m_MSRecord.begin());
		}
	}
	void InfoPanel::OnImGui()
	{
		{
			ImGui::Begin(m_Name.c_str(), &m_Open);
			ImGui::Text("Application name: %s", Zephyr::Application::Get().Specification().Name.c_str());
			if (ImGui::CollapsingHeader("Renderer"))
			{
				Zephyr::RenderDevice device = Zephyr::Renderer::GetRenderDevice();
				m_VramUsage.emplace_back(device.UsedVRAM);
				if (m_VramUsage.size() > 100)
				{
					m_VramUsage.erase(m_VramUsage.begin());
				}


				ImGui::Text("Graphics API: %s", Zephyr::GetGraphicsName(Zephyr::Application::Get().Specification().WindowData.API).data());
				ImGui::Text("Rendering path: ");
				ImGui::Separator();
				ImGui::Text("Device: %s", device.Name.data());
				ImGui::Text("Vendor: %s", device.Vendor.data());
				ImGui::Text("Application used VRAM: %i/%i (MB)", device.UsedVRAM, device.AvailableVRAM);
				ImGui::PlotLines("##vram", &m_VramUsage[0], (int)m_VramUsage.size(), 0, "VRAM Usage", 0.0f, 1000.0f, ImVec2(200, 100));

				ImGui::Text("Total VRAM: %i (MB)", device.TotalVRAM);
			}
			if (ImGui::CollapsingHeader("Frame rate"))
			{
				ImGui::PlotLines("##fps", &m_FPSRecord[0], (int)m_FPSRecord.size(), 0, "FPS", 0.0f, 1000.0f, ImVec2(200, 100));

				ImGui::Text("FPS: %.3f", Zephyr::Time::GetFPS());

				ImGui::PlotLines("##ms", &m_MSRecord[0], (int)m_MSRecord.size(), 0, "Delta time", 0.0f, 1000.0f, ImVec2(200, 100));

				ImGui::Text("Delta time: %.3f ms", Zephyr::Time::GetDeltaTime());
			}


			ImGui::End();
		}
	}
}
