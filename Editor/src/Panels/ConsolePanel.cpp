#include "ConsolePanel.h"
#include <Zephyr/FileSystem/FileSystem.h>
#include <Zephyr/FileSystem/FileDialogs.h>

namespace Editor
{
	ConsolePanel::ConsolePanel()
		: Panel("Console", PanelCategory::WINDOW)
	{
		m_AutoScroll = true;
		Clear();

		m_LevelColors[Zephyr::LogLevel::TRACE] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_LevelColors[Zephyr::LogLevel::INFO] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
		m_LevelColors[Zephyr::LogLevel::WARN] = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
		m_LevelColors[Zephyr::LogLevel::ERR] = ImVec4(0.5f, 0.0f, 0.0f, 1.0f);
		m_LevelColors[Zephyr::LogLevel::CRITICAL] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

		Zephyr::Log::SetLogCallback([&](Zephyr::LogLevel level, Zephyr::String string) {

			AddLog(level, string.c_str());
			});
	}
	void ConsolePanel::OnUpdate()
	{
	}
	void ConsolePanel::OnImGui()
	{
		ImGui::Begin(m_Name.c_str(), &m_Open);
		// Options menu
		if (ImGui::BeginPopup("Options"))
		{
			ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
			ImGui::EndPopup();
		}

		// Main window
		if (ImGui::Button("Options"))
		{
			ImGui::OpenPopup("Options");
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
		{
			Clear();
		}
		ImGui::SameLine();
		if (ImGui::Button("Copy"))
		{
			ImGui::LogToClipboard();
		}
		ImGui::SameLine();
		if (ImGui::Button("Save log"))
		{
			auto savePath = Zephyr::FileDialogs::SaveFile("Log file (*.log)\0.log\0");
			savePath.replace_extension(".log");
			Zephyr::Buffer buffer(m_Buf.c_str(), m_Buf.size());

			Zephyr::File file(savePath, Zephyr::File::OpenMode::OUTPUT);
			file.Write(buffer);
		}

		ImGui::SameLine();
		m_Filter.Draw("Filter", -100.0f);

		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);



		if (m_Buf.size() > 0)
		{

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			const char* buf = m_Buf.begin();
			const char* buf_end = m_Buf.end();
			if (m_Filter.IsActive())
			{
				// In this example we don't use the clipper when Filter is enabled.
				// This is because we don't have random access to the result of our filter.
				// A real application processing logs with ten of thousands of entries may want to store the result of
				// search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
				for (int line_no = 0; line_no < m_LineOffsets.Size; line_no++)
				{
					const char* line_start = buf + m_LineOffsets[line_no];
					const char* line_end = (line_no + 1 < m_LineOffsets.Size) ? (buf + m_LineOffsets[line_no + 1] - 1) : buf_end;
					if (m_Filter.PassFilter(line_start, line_end)) {
						ImGui::PushStyleColor(ImGuiCol_Text, m_LevelColors[m_LogLevels[line_no]]);
						ImGui::TextUnformatted(line_start, line_end);
						ImGui::PopStyleColor();
					}
				}
			}
			else
			{
				// The simplest and easy way to display the entire buffer:
				//   ImGui::TextUnformatted(buf_begin, buf_end);
				// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
				// to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
				// within the visible area.
				// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
				// on your side is recommended. Using ImGuiListClipper requires
				// - A) random access into your data
				// - B) items all being the  same height,
				// both of which we can handle since we have an array pointing to the beginning of each line of text.
				// When using the filter (in the block of code above) we don't have random access into the data to display
				// anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
				// it possible (and would be recommended if you want to search through tens of thousands of entries).
				ImGuiListClipper clipper;
				clipper.Begin(m_LineOffsets.Size - 1);
				while (clipper.Step())
				{
					for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
					{
						const char* line_start = buf + m_LineOffsets[line_no];
						const char* line_end = (line_no + 1 < m_LineOffsets.Size) ? (buf + m_LineOffsets[line_no + 1] - 1) : buf_end;
						ImGui::PushStyleColor(ImGuiCol_Text, m_LevelColors[m_LogLevels[line_no]]);
						ImGui::TextUnformatted(line_start, line_end);
						ImGui::PopStyleColor();
					}
				}
				clipper.End();
			}
			ImGui::PopStyleVar();

			if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
		}

		ImGui::EndChild();

		ImGui::End();
	}
	void ConsolePanel::Clear()
	{
		m_Buf.clear();
		m_LogLevels.clear();
		m_LineOffsets.clear();
		m_LineOffsets.push_back(0);

		m_InfoCount = m_WarnCount = m_ErrorCount = 0;
	}

	void ConsolePanel::AddLog(Zephyr::LogLevel level, const char* fmt, ...) IM_FMTARGS(2)
	{
		int old_size = m_Buf.size();
		va_list args;
		va_start(args, fmt);
		m_Buf.appendfv(fmt, args);
		va_end(args);
		for (int new_size = m_Buf.size(); old_size < new_size; old_size++)
		{
			if (m_Buf[old_size] == '\n') 
			{
				m_LineOffsets.push_back(old_size + 1);
				m_LogLevels.push_back(level);
			}
		}
		if (level == Zephyr::LogLevel::INFO || level == Zephyr::LogLevel::TRACE) m_InfoCount++;
		else if (level == Zephyr::LogLevel::WARN) m_WarnCount++;
		else if (level == Zephyr::LogLevel::ERR || level == Zephyr::LogLevel::CRITICAL) m_ErrorCount++;
	}
}