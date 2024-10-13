#include <pch.h>
#include "AssetBrowserPanel.h"
#include <imgui.h>
#include <Zephyr/Project/Project.h>
#include <Zephyr/FileSystem/FileDialogs.h>

#include <FontIcons/IconsForkAwesome.h>

namespace Editor
{
	namespace
	{
		Zephyr::Path AbsolutePath = "Assets";
		Zephyr::Path LastDir = AbsolutePath;
		Zephyr::Path LastDirCopy = AbsolutePath;

		ImGuiTextFilter FileFilter;
		ImGuiTextFilter FolderFilter;

		bool ShowFolders = true;
		std::filesystem::file_time_type LastCurrentDirWriteTime;
		std::filesystem::file_time_type LastAbsoluteDirWriteTime;

		std::vector<Folder> AbsoluteFolders;
		Folder CurrentDirFolder;
		ImGuiTreeNodeFlags FolderNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

		bool RenameAssets = false;
		Zephyr::Path SelectedPath{};

		void GetChildFolders(Folder* folder)
		{
			folder->Children.clear();
			for (const auto& directoryEntry : std::filesystem::directory_iterator(folder->Path))
			{
				if (!directoryEntry.is_directory())
					continue;
				Folder child{};
				child.Path = directoryEntry.path();
				folder->Children.push_back(child);
				GetChildFolders(&child);
			}
		}
		void GetFolderFiles(Folder* folder)
		{
			folder->Files.clear();
			for (const auto& directoryEntry : std::filesystem::directory_iterator(folder->Path))
			{
				if (directoryEntry.is_directory())
					continue;
				File file;
				file.Path = directoryEntry.path();
				folder->Files.push_back(file);
			}
		}


	}
	AssetBrowserPanel::AssetBrowserPanel()
		: Panel("Asset Browser", PanelCategory::WINDOW)
	{
		m_CurrentDir = AbsolutePath;
		UpdateDirectories();
		UpdateFolders();
	}
	void AssetBrowserPanel::OnUpdate()
	{
		// Check directories time stamps
		if (m_CurrentDir != LastDir || std::filesystem::last_write_time(m_CurrentDir) != LastCurrentDirWriteTime)
		{
			UpdateDirectories();
			UpdateFolders();
			LastCurrentDirWriteTime = std::filesystem::last_write_time(m_CurrentDir);
		}
		if (std::filesystem::last_write_time(AbsolutePath) != LastAbsoluteDirWriteTime)
		{
			UpdateFolders();
			UpdateDirectories();
			LastAbsoluteDirWriteTime = std::filesystem::last_write_time(AbsolutePath);
		}

		// Check if the absolute path exists and it's the current project absolute path
		if (AbsolutePath != Zephyr::Project::GetActive()->GetWorkingDirectory() && Zephyr::FileSystem::Exists(Zephyr::Project::GetActive()->GetWorkingDirectory()))
		{
			AbsolutePath = Zephyr::Project::GetActive()->GetWorkingDirectory();
			m_CurrentDir = AbsolutePath;
			UpdateDirectories();
			UpdateFolders();
		}
	}


	void AssetBrowserPanel::UpdateDirectories()
	{
		if (std::filesystem::exists(LastDir))
			LastDirCopy = LastDir;
		if (std::filesystem::exists(m_CurrentDir))
			LastDir = m_CurrentDir;

		Folder currentFolder{};
		currentFolder.Path = m_CurrentDir;
		for (const auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDir))
		{
			if (directoryEntry.is_directory())
			{
				Folder folder;
				folder.Path = directoryEntry.path();
				GetChildFolders(&folder);
				GetFolderFiles(&folder);
				for (auto& child : folder.Children)
				{
					GetFolderFiles(&child);
				}
				currentFolder.Children.push_back(folder);
			}
			else
			{
				File file{};
				file.Path = directoryEntry.path();
				currentFolder.Files.push_back(file);
			}
		}
		CurrentDirFolder = currentFolder;
	}

	void AssetBrowserPanel::UpdateFolders()
	{
		AbsoluteFolders.clear();
		for (const auto& directoryEntry : std::filesystem::directory_iterator(AbsolutePath))
		{
			if (!directoryEntry.is_directory())
				continue;

			Folder folder{};
			folder.Path = directoryEntry.path();
			GetChildFolders(&folder);
			GetFolderFiles(&folder);
			for (auto& child : folder.Children)
			{
				GetFolderFiles(&child);
			}
			AbsoluteFolders.push_back(folder);
		}
	}

	void AssetBrowserPanel::DrawFolder(const Folder& folder)
	{
		const std::string fileName(ICON_FK_FOLDER + folder.Path.filename().string());
		ImGuiTreeNodeFlags nodeFlags = folder.Children.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : FolderNodeFlags;
		if (m_CurrentDir == folder.Path)
			nodeFlags |= ImGuiTreeNodeFlags_Selected;

		if (FolderFilter.PassFilter(folder.Path.filename().string().c_str()))
		{
			bool open = ImGui::TreeNodeEx(fileName.c_str(), nodeFlags);
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
				m_CurrentDir = folder.Path;

			if (open)
			{
				for (const auto& child : folder.Children)
				{
					DrawFolder(child);
				}
				if (!folder.Children.empty())
					ImGui::TreePop();
			}
			ImGui::TableNextColumn();
		}
	}

	void AssetBrowserPanel::DrawFile(File& file, float thumbnailSize, float padding)
	{
		if (SelectedPath == file.Path)
			ImGui::SetItemDefaultFocus();
		const float hpadding = padding / 2.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { hpadding, hpadding });
		if (ImGui::Button(file.Path.string().c_str(), {thumbnailSize, thumbnailSize})) {
			SelectedPath = file.Path;
		}
		ImGui::PopStyleVar();

		if (ImGui::BeginDragDropSource()) {

			ImGui::SetDragDropPayload("ASSET_ITEM", file.Path.string().c_str(), file.Path.string().size() + 1);
			ImGui::Text(file.Path.string().c_str());
			ImGui::Text(file.Path.filename().string().c_str());
			ImGui::EndDragDropSource();
		}
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			Zephyr::FileDialogs::OpenFile(file.Path.string().c_str());
		}
		ImGui::TextWrapped(file.Path.filename().string().c_str());
	}



	void AssetBrowserPanel::TopBar()
	{
		if (ImGui::BeginMenuBar())
		{
			ImGui::Checkbox("Show Folders", &ShowFolders);
			ImGui::EndMenuBar();
		}
	}

	void AssetBrowserPanel::FolderBrowser()
	{
		ImGui::PushID(0);
		if (ImGui::Button(ICON_FK_REFRESH))
		{
			UpdateFolders();
		}
		ImGui::PopID();
		ImGui::SameLine();
		FolderSearch();
		ImGui::Separator();
		if (ImGui::BeginTable("##folder_browser", 1))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			for (auto& folder : AbsoluteFolders)
				DrawFolder(folder);

			ImGui::EndTable();
		}
		ImGui::TableNextColumn();
	}

	void AssetBrowserPanel::DrawFolder(const float cellSize, int& id, Folder& folder)
	{
		ImGui::PushID(id++);
		if (SelectedPath == folder.Path)
			ImGui::SetItemDefaultFocus();
		if (ImGui::Button(ICON_FK_FOLDER, { cellSize, cellSize }))
		{
			SelectedPath = folder.Path;
		}
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			m_CurrentDir = folder.Path;
		}
		else
			ImGui::TextWrapped(folder.Path.filename().string().c_str());
		ImGui::TableNextColumn();
		ImGui::PopID();
	}

	void AssetBrowserPanel::FileBrowser()
	{
		bool on_parent_dir = m_CurrentDir == AbsolutePath;

		ImGui::BeginDisabled(on_parent_dir);
		if (ImGui::Button(ICON_FK_BACKWARD))
		{
			m_CurrentDir = m_CurrentDir.parent_path();
		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		ImGui::BeginDisabled(LastDirCopy == AbsolutePath || std::filesystem::exists(LastDirCopy));
		if (ImGui::Button(ICON_FK_FORWARD))
		{
			m_CurrentDir = LastDirCopy;
		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		ImGui::SameLine();
		ImGui::Text(on_parent_dir ?
			"Assets" :
			(std::filesystem::path("Assets") /= std::filesystem::relative(m_CurrentDir, AbsolutePath)).string().c_str()
		);
		ImGui::SameLine();
		ImGui::PushID(1);
		if (ImGui::Button(ICON_FK_REFRESH))
		{
			UpdateDirectories();
		}
		ImGui::PopID();
		ImGui::SameLine();
		FileSearch();
		ImGui::Separator();

		const float padding = 16.0f * m_ButtonSize;
		const float thumbnailSize = 64.0f * m_ButtonSize;
		const float cellSize = (thumbnailSize + padding);

		const float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		if (ImGui::BeginTable("##assets", columnCount))
		{
			int id = 0;
			ImGui::TableNextColumn();

			for (auto& file : CurrentDirFolder.Files)
			{
				if (FileFilter.PassFilter(file.Path.string().c_str()))
				{
					ImGui::PushID(id++);
					DrawFile(file, thumbnailSize, padding);
					ImGui::PopID();
					ImGui::TableNextColumn();
				}
			}
			if (ShowFolders)
			{
				for (auto& folder : CurrentDirFolder.Children)
				{
					if (FileFilter.PassFilter(folder.Path.string().c_str()))
					{
						DrawFolder(cellSize, id, folder);
					}
				}
			}
			if (ImGui::BeginPopupContextWindow("##assets_context_window"))
			{
				if (ImGui::BeginMenu("Create"))
				{
					if (ImGui::MenuItem("Folder"))
					{
						std::filesystem::create_directory(m_CurrentDir / "New folder");
					}
					ImGui::EndMenu();
				}
				if (exists(SelectedPath))
				{
					ImGui::Text(SelectedPath.filename().string().c_str());
					ImGui::Separator();
					if (ImGui::MenuItem(ICON_FK_TRASH " Delete"))
					{
						std::filesystem::remove(SelectedPath);
						SelectedPath = "";
					}
				}

				if (ImGui::MenuItem("Open in file explorer"))
				{
					Zephyr::FileDialogs::OpenFile(m_CurrentDir.string().c_str());
				}

				ImGui::EndPopup();
			}

			ImGui::EndTable();
		}
	}
	void AssetBrowserPanel::FolderSearch()
	{
		FolderFilter.Draw("##folder_search");
	}
	void AssetBrowserPanel::FileSearch()
	{
		FileFilter.Draw("##file_search");
	}
	void AssetBrowserPanel::SearchForFile(const Folder& folder)
	{
		for (const auto& child : folder.Children)
		{
			SearchForFile(child);
		}
		for (const File& file : folder.Files)
		{
			if (FileFilter.PassFilter(file.Path.filename().string().c_str()))
			{
				if (ImGui::MenuItem(file.Path.string().c_str()))
				{
					m_CurrentDir = file.Path;
				}
			}
		}

	}
	void AssetBrowserPanel::OnImGui()
	{
		ImGui::Begin(m_Name.c_str(), &m_Open, ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginTable("##content_browser", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextColumn();
			TopBar();
			FolderBrowser();
			FileBrowser();

			ImGui::EndTable();
		}


		ImGui::End();
	}


}