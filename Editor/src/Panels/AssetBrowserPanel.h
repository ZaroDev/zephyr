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
		AssetBrowserPanel();
		virtual ~AssetBrowserPanel() = default;

		void OnUpdate() override;
		void OnImGui() override;

	private:
		void UpdateDirectories();
		void UpdateFolders();



		void DrawFolder(const Folder& folder);
		void DrawFolder(float cellSize, int& id, Folder& folder);
		void DrawFile(File& file, float thumbnailSize, float padding);
		void TopBar();
		void FolderBrowser();
		void FileBrowser();
		void SearchForFile(const Folder& folder);
		void FileSearch();
		void FolderSearch();
	private:
		Zephyr::Path m_CurrentDir;
		Zephyr::Path m_LastDir;

		float m_ButtonSize = 1.f;
	};
}