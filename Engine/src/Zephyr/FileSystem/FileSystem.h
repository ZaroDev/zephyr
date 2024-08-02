#pragma once

namespace Zephyr::FileSystem
{

	bool Exists(const Path& path);
	
	size FileSize(const Path& path);
	
	Path WorkingDirectory();
	
	void WorkingDirectory(const Path& path);
	
	bool IsEmpty(const Path& path);
	
	bool IsEquivalent(const Path& path1, const Path& path2);
	
	bool CreateFolder(const Path& path);
	
	bool CreateFolders(const Path& path);
	
	void CreateDirectorySymlink(const Path& to, const Path& symlink);
	
	void CreateHardlink(const Path& to, const Path& hardlink);

	void CreateSymlink(const Path& to, const Path& symlink);

	bool Remove(const Path& path);

	bool RemoveAll(const Path& path);

	Path ReplaceExtension(const Path& path, std::string_view extension);

	Path GetFileName(const Path& path);

	bool IsDirectory(const Path& path);
}