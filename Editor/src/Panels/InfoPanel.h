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

	private:
		std::vector<float> m_VramUsage;
		std::vector<float> m_FPSRecord;
		std::vector<float> m_MSRecord;

		float m_Counter = 0;
	};
}