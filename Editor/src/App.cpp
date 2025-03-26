#include "App.h"
#include <imgui.h>
#include <Panels/Panels.h>

#include <FontIcons/IconsForkAwesome.h>
#include <Zephyr/Project/Project.h>

namespace Editor
{
	void Application::OnInit()
	{
		SetImGuiTheme();
		m_Scene = Zephyr::CreateRef<Zephyr::ECS::Scene>();


		m_Panels.emplace_back(Zephyr::CreateScope<InfoPanel>());
		m_Panels.emplace_back(Zephyr::CreateScope<ProjectPanel>());
		m_Panels.emplace_back(Zephyr::CreateScope<ConsolePanel>());
		m_Panels.emplace_back(Zephyr::CreateScope<HierarchyPanel>());
		m_Panels.emplace_back(Zephyr::CreateScope<AssetBrowserPanel>());
		m_Panels.emplace_back(Zephyr::CreateScope<ScenePanel>());

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
			if (ImGui::BeginMenu("About"))
			{
				if (ImGui::MenuItem("Load dummy texture"))
				{
					m_Textures.emplace_back(Zephyr::TextureImporter::LoadTexture2D("Resources/test.png"));
				}
				if (ImGui::MenuItem("Load model"))
				{
					
				}

				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
		
	}
	void Application::SetImGuiTheme()
	{
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("Resources/Fonts/CascadiaCode.ttf", 16.0f);
		{
			const ImWchar icons_ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
			ImFontConfig icons_config;
			icons_config.MergeMode = true;
			icons_config.GlyphMinAdvanceX = 16.0f;
			io.Fonts->AddFontFromFileTTF("Resources/Fonts/forkawesome-webfont.ttf", 16.0f, &icons_config, icons_ranges);
		}
		io.Fonts->Build();

		m_Renderer.RebuildFontTextures();
	}
}

namespace Zephyr
{
	Application* CreateApplication(const ApplicationCommandLineArgs& args)
	{
		ApplicationSpecification spec;
		spec.Args = args;
		spec.WindowData.API = GraphicsAPI::VULKAN;
		spec.Name = "Zephyr Editor";

		return new Editor::Application(spec);
	}
}