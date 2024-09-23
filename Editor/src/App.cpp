#include "App.h"
#include <imgui.h>
#include <Panels/Panels.h>

namespace Editor
{
	void Application::OnInit()
	{
		m_Panels.emplace_back(Zephyr::CreateScope<InfoPanel>());
		m_Panels.emplace_back(Zephyr::CreateScope<ProjectPanel>());
	}
	void Application::OnUpdate()
	{
		for (auto& panel : m_Panels)
		{
			panel->OnUpdate();
		}
	}
	void Application::OnImGuiUpdate()
	{
		MainMenuBar();
		DockSpace();

		for (auto& panel : m_Panels)
		{
			if (panel->IsActive())
			{
				panel->OnImGui();
			}
		}
	}
	void Application::OnShutdown()
	{
	}

	void Application::DockSpace()
	{
		// Note: Switch this to true to enable dockspace
		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("DockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}


		ImGui::End();
	}

	void Application::MainMenuBar()
	{
		if(ImGui::BeginMainMenuBar())
		{
			if(ImGui::BeginMenu("Window"))
			{
				for(auto& panel : m_Panels)
				{
					if(panel->GetCategory() == PanelCategory::WINDOW)
					{
						if(ImGui::MenuItem(panel->GetName().data(), nullptr, panel->IsActive()))
						{
							panel->SwitchActive();
						}
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Info"))
			{
				for (auto& panel : m_Panels)
				{
					if (panel->GetCategory() == PanelCategory::INFO)
					{
						if (ImGui::MenuItem(panel->GetName().data(), nullptr, panel->IsActive()))
						{
							panel->SwitchActive();
						}
					}
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}
}

namespace Zephyr
{
	Application* CreateApplication(const ApplicationCommandLineArgs& args)
	{
		ApplicationSpecification spec;
		spec.Args = args;

#ifdef PLATFORM_WINDOWS
		spec.API = GraphicsAPI::DX11;
#else
		spec.API = GraphicsAPI::OPENGL;
#endif
		spec.Name = "Zephyr Editor";

		return new Editor::Application(spec);
	}
}