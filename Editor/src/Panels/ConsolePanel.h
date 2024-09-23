#pragma once

#include "Panel.h"
#include "imgui.h"

namespace Editor
{
	class ConsolePanel final : public Panel
	{
	public:
		ConsolePanel();

		void OnUpdate() override;
		void OnImGui() override;

	private:
		void Clear();

	private:


		ImGuiTextBuffer     Buf;
		ImGuiTextFilter     Filter;
		ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
		ImVector<LogLevel>  LogLevels;
		bool                AutoScroll;  // Keep scrolling if already at the bottom.

		ImVec4 LevelColors[LogLevel::LAST];
		int infoCount;
		int warnCount;
		int errorCount;
	};
}