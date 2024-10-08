#pragma once

#include "Panel.h"

namespace Editor
{
	class InfoPanel final : public Panel
	{
	public:
		InfoPanel() : Panel("Info", PanelCategory::INFO){}

		void OnUpdate() override;
		void OnImGui() override;
	};
}