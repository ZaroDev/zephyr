#pragma once
#include "Panel.h"

namespace Editor
{
	
	class CVarPanel : public Panel
	{
	public:
		CVarPanel()
			: Panel("CVar", PanelCategory::WINDOW) 
		{
		}

		virtual ~CVarPanel() = default;

		virtual void OnUpdate() override;
		virtual void OnImGui()  override;
	};
}