#pragma once

#include <imgui.h>
#include <ImGuizmo.h>

#include "Panel.h"
#include "Zephyr/ECS/Entity.h"

namespace Editor
{
	class HierarchyPanel final : public Panel
	{
	public:
		HierarchyPanel() : Panel("Hierarchy", PanelCategory::WINDOW)
		{
		}

		virtual ~HierarchyPanel() = default;

		void OnUpdate() override;
		void OnImGui() override;

	private:

		void DrawGizmos();
		void DrawEntityNode(Zephyr::ECS::Entity entity);
		void DrawComponents(Zephyr::ECS::Entity entity);
		template<typename T>
		void DisplayAddComponentEntry(const std::string& entryName);

		Zephyr::Ref<Zephyr::ECS::Scene> m_Context;
		Zephyr::ECS::Entity m_SelectionContext;
		ImGuizmo::OPERATION m_GizmoType = ImGuizmo::BOUNDS;
		ImGuizmo::MODE m_GizmoMode = ImGuizmo::LOCAL;
	};
}
