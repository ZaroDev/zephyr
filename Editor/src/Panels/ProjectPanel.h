#pragma once

#include "Panel.h"

namespace Editor
{
	class ProjectPanel final : public Panel
	{
	public:
		ProjectPanel() : Panel("Project", PanelCategory::WINDOW) {}

		void OnUpdate() override;
		void OnImGui() override;

	};
}