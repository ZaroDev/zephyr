#pragma once

#include "Panel.h"
#include "imgui.h"
#include <Zephyr/Core/Log.h>

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
		void AddLog(Zephyr::LogLevel level, const char* fmt, ...) IM_FMTARGS(2);
	private:


		ImGuiTextBuffer     Buf;
		ImGuiTextFilter     Filter;
		ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
		ImVector<Zephyr::LogLevel>  LogLevels;
		bool                AutoScroll;  // Keep scrolling if already at the bottom.

		ImVec4 LevelColors[Zephyr::LogLevel::LAST];
		int infoCount;
		int warnCount;
		int errorCount;
	};
}