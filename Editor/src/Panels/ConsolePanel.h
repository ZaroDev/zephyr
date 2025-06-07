#pragma once

#include "Panel.h"
#include "imgui.h"
#include <Zephyr/Core/BasicTypes.h>
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


		ImGuiTextBuffer     m_Buf;
		ImGuiTextFilter     m_Filter;
		ImVector<u32>       m_LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
		ImVector<Zephyr::LogLevel>  m_LogLevels;
		bool                m_AutoScroll;  // Keep scrolling if already at the bottom.

		ImVec4 m_LevelColors[Zephyr::LogLevel::LAST];
		u32 m_InfoCount;
		u32 m_WarnCount;
		u32 m_ErrorCount;
	};
}