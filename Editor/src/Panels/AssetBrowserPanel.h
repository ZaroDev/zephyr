#pragma once
#include "Panel.h"

namespace Editor
{
	struct File
	{
		Zephyr::Path Path;
	};

	struct Folder
	{
		Zephyr::Path Path;
		std::vector<Folder> Children;
		std::vector<File> Files;
	};

	class AssetBrowserPanel final : public Panel
	{
	public:
		AssetBrowserPanel() : Panel("Asset Browser", PanelCategory::WINDOW)
		{
			UpdateDirectories();
			UpdateFolders();
		}

		void OnUpdate() override;
		void OnImGui() override;

	private:
		void UpdateDirectories();
		void UpdateFolders();




		void DrawFolder(float cellSize, int& id, Folder& folder);
		void DrawFileBrowser();
		void DrawFolderSearch();
		void DrawFileSearch();


	private:
		Zephyr::Path m_CurrentDir;
		Zephyr::Path m_LastDir;


	};
}