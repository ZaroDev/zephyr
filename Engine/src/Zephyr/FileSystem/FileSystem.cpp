#include <pch.h>
#include "FileSystem.h"

namespace Zephyr::FileSystem
{
	bool Exists(const Path& path)
	{
		return std::filesystem::exists(path);
	}
	size FileSize(const Path& path)
	{
		CORE_ASSERT(Exists(path) && IsDirectory(path), "File doesn't exists");
		return std::filesystem::file_size(path);
	}
	Path WorkingDirectory()
	{
		return std::filesystem::current_path();
	}
	void WorkingDirectory(const Path& path)
	{
		CORE_ASSERT(Exists(path), "Path doesn't exist");
		std::filesystem::current_path(path);
	}
	bool IsEmpty(const Path& path)
	{
		CORE_ASSERT(Exists(path), "Path doesn't exist");
		return std::filesystem::is_empty(path);
	}
	bool IsEquivalent(const Path& path1, const Path& path2)
	{
		CORE_ASSERT(Exists(path1) && Exists(path2), "Path doesn't exist");
		return std::filesystem::equivalent(path1, path2);
	}
	bool CreateFolder(const Path& path)
	{
		CORE_ASSERT(!Exists(path), "Path already exists!");
		return std::filesystem::create_directory(path);
	}
	bool CreateFolders(const Path& path)
	{
		CORE_ASSERT(!Exists(path), "Path already exists!");
		return std::filesystem::create_directories(path);
	}
	void CreateDirectorySymlink(const Path& to, const Path& symlink)
	{
		std::filesystem::create_directory_symlink(to, symlink);
	}
	void CreateHardlink(const Path& to, const Path& hardlink)
	{
		std::filesystem::create_hard_link(to, hardlink);
	}
	void CreateSymlink(const Path& to, const Path& symlink)
	{
		std::filesystem::create_symlink(to, symlink);
	}
	bool Remove(const Path& path)
	{
		CORE_ASSERT(Exists(path), "Path doesn't exist");
		return std::filesystem::remove(path);
	}
	bool RemoveAll(const Path& path)
	{
		CORE_ASSERT(Exists(path), "Path doesn't exist");
		return std::filesystem::remove_all(path);
	}
	Path ReplaceExtension(const Path& path, std::string_view extension)
	{
		Path ret = path;
		ret.replace_extension(extension);
		return ret;
	}
	Path GetFileName(const Path& path)
	{
		return path.filename();
	}
	bool IsDirectory(const Path& path)
	{
		return std::filesystem::is_directory(path);
	}
}