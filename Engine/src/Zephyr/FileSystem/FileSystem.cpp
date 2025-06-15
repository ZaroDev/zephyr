#include <pch.h>
#include "FileSystem.h"

#include "Utils/StringUtils.h"

namespace Zephyr
{
	Blob::Blob(void* data, SizeT size)
		: m_Data(data), m_Size(size)
	{
	}

	Blob::~Blob()
	{
		if (m_Data)
		{
			delete m_Data;
			m_Data = nullptr;
		}

		m_Size = 0;
	}

	const void* Blob::Data() const
	{
		return m_Data;
	}

	SizeT Blob::Size() const
	{
		return m_Size;
	}



	bool DefaultFileSystem::FolderExists(const Path& path)
	{
		return std::filesystem::exists(path) && std::filesystem::is_directory(path);
	}

	bool DefaultFileSystem::FileExists(const Path& path)
	{
		return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
	}

	Ref<IBlob> DefaultFileSystem::ReadFile(const Path& path)
	{
		if (!FileExists(path))
		{
			CORE_ASSERT(false, "Error file doesn't exist");
			CORE_ERROR("[DefaultFileSystem]: Error {} doesn't exist!", path.generic_string().c_str());
			return nullptr;
		}

		std::ifstream file(path, std::ios::binary);
		if (!file.is_open())
		{
			CORE_ERROR("[DefaultFileSystem]: Error failed to open file {}!", path.generic_string().c_str());
			return nullptr;
		}

		file.seekg(0, std::ios::end);
		u64 size = file.tellg();
		file.seekg(0, std::ios::beg);
		if (size > static_cast<u64> (std::numeric_limits<SizeT>::max()))
		{
			CORE_ASSERT(false);
			CORE_ERROR("[DefaultFileSystem]: Error failed to read file {} size too long!", path.generic_string().c_str());
			return nullptr;
		}

		Byte* data = static_cast<Byte*>(std::malloc(size));

		if (data == nullptr)
		{
			CORE_ASSERT(false);
			return nullptr;
		}

		file.read(data, size);
		if (!file.good())
		{
			CORE_ASSERT(false);
			std::free(data);
			return nullptr;
		}

		return CreateRef<Blob>(data, size);
	}

	bool DefaultFileSystem::WriteFile(const Path& path, const void* data, SizeT size)
	{
		std::ofstream file(path, std::ios::binary);
		if (!file.is_open())
		{
			CORE_ERROR("[DefaultFileSystem]: Error failed to open file {}!", path.generic_string().c_str());
			return false;
		}

		
		if (size > 0)
		{
			file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
		}

		if (!file.good())
		{
			CORE_ERROR("[DefaultFileSystem]: Error failed to write file {}!", path.generic_string().c_str());
			return false;
		}

		return true;
	}
	static int enumerateNativeFiles(const char* pattern, bool directories, EnumerateCallbackFunc callback)
	{
#ifdef WIN32

		WIN32_FIND_DATAA findData;
		HANDLE hFind = FindFirstFileA(pattern, &findData);

		if (hFind == INVALID_HANDLE_VALUE)
		{
			if (GetLastError() == ERROR_FILE_NOT_FOUND)
				return 0;

			return Status::Failed;
		}

		int numEntries = 0;

		do
		{
			bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			bool isDot = strcmp(findData.cFileName, ".") == 0;
			bool isDotDot = strcmp(findData.cFileName, "..") == 0;

			if ((isDirectory == directories) && !isDot && !isDotDot)
			{
				callback(findData.cFileName);
				++numEntries;
			}
		} while (FindNextFileA(hFind, &findData) != 0);

		FindClose(hFind);

		return numEntries;

#else // WIN32

		glob64_t glob_matches;
		int globResult = glob64(pattern, 0 /*flags*/, nullptr /*errfunc*/, &glob_matches);

		if (globResult == 0)
		{
			int numEntries = 0;

			for (int i = 0; i < glob_matches.gl_pathc; ++i)
			{
				const char* globentry = (glob_matches.gl_pathv)[i];
				std::error_code ec, ec2;
				std::filesystem::directory_entry entry(globentry, ec);
				if (!ec)
				{
					if (directories == entry.is_directory(ec2) && !ec2)
					{
						callback(entry.path().filename().native());
						++numEntries;
					}
				}
			}
			globfree64(&glob_matches);

			return numEntries;
		}

		if (globResult == GLOB_NOMATCH)
			return 0;

		return Status::Failed;

#endif // WIN32
	}
	i32 DefaultFileSystem::EnumerateFiles(const Path& path, const std::vector<String>& extensions,
		EnumerateCallbackFunc callback, bool allowDuplicates)
	{
		(void)allowDuplicates;
		if (extensions.empty())
		{
			std::string pattern = (path / "*").generic_string();
			return enumerateNativeFiles(pattern.c_str(), false, callback);
		}

		int numEntries = 0;
		for (const auto& ext : extensions)
		{
			std::string pattern = (path / ("*" + ext)).generic_string();
			int result = enumerateNativeFiles(pattern.c_str(), false, callback);

			if (result < 0)
				return result;

			numEntries += result;
		}

		return numEntries;
	}

	i32 DefaultFileSystem::EnumerateDirectories(const Path& path, EnumerateCallbackFunc callback, bool allowDuplicates)
	{
		(void)allowDuplicates;

		std::string pattern = (path / "*").generic_string();
		return enumerateNativeFiles(pattern.c_str(), true, callback);
	}
	RelativeFileSystem::RelativeFileSystem(Scope<IFileSystem> fs, const Path& basePath)
		: m_UnderlyingFS(std::move(fs))
		, m_BasePath(basePath.lexically_normal())
	{
	}


	bool RelativeFileSystem::FolderExists(const Path& name)
	{
		return m_UnderlyingFS->FolderExists(m_BasePath / name.relative_path());
	}

	bool RelativeFileSystem::FileExists(const Path& name)
	{
		return m_UnderlyingFS->FileExists(m_BasePath / name.relative_path());
	}

	Ref<IBlob> RelativeFileSystem::ReadFile(const Path& name)
	{
		return m_UnderlyingFS->ReadFile(m_BasePath / name.relative_path());
	}

	bool RelativeFileSystem::WriteFile(const Path& name, const void* data, size_t size)
	{
		return m_UnderlyingFS->WriteFile(m_BasePath / name.relative_path(), data, size);
	}

	int RelativeFileSystem::EnumerateFiles(const Path& path, const std::vector<std::string>& extensions, EnumerateCallbackFunc callback, bool allowDuplicates)
	{
		return m_UnderlyingFS->EnumerateFiles(m_BasePath / path.relative_path(), extensions, callback, allowDuplicates);
	}

	int RelativeFileSystem::EnumerateDirectories(const Path& path, EnumerateCallbackFunc callback, bool allowDuplicates)
	{
		return m_UnderlyingFS->EnumerateDirectories(m_BasePath / path.relative_path(), callback, allowDuplicates);
	}

	void RootFileSystem::Mount(const Path& path, Ref<IFileSystem> fs)
	{
		if (FindMountPoint(path, nullptr, nullptr))
		{
			CORE_ERROR("[RootFileSystem]: Cannot mount a filesystem at {}: there is another FS that includes this path", path.string().c_str());
			return;
		}

		m_MountingPoints.emplace_back(path.lexically_normal().generic_string(), fs);
	}

	void RootFileSystem::Mount(const Path& path, const Path& nativePath)
	{
		Mount(path, CreateRef<RelativeFileSystem>(CreateScope<DefaultFileSystem>(), nativePath));
	}

	bool RootFileSystem::UnMount(const Path& path)
	{
		String spath = path.lexically_normal().generic_string();

		for (SizeT index = 0; index < m_MountingPoints.size(); index++)
		{
			if (m_MountingPoints[index].first == spath)
			{
				m_MountingPoints.erase(m_MountingPoints.begin() + index);
				return true;
			}
		}

		return false;
	}

	bool RootFileSystem::FindMountPoint(const Path& path, Path* pRelativePath, IFileSystem** ppFS)
	{
		String spath = path.lexically_normal().generic_string();

		for (auto it : m_MountingPoints)
		{
			if (spath.find(it.first, 0) == 0 && ((spath.length() == it.first.length()) || (spath[it.first.length()] == '/')))
			{
				if (pRelativePath)
				{
					String relative = (spath.length() == it.first.length()) ? "" : spath.substr(it.first.size() + 1);
					*pRelativePath = relative;
				}

				if (ppFS)
				{
					*ppFS = it.second.get();
				}

				return true;
			}
		}

		return false;
	}

	bool RootFileSystem::FolderExists(const Path& name)
	{
		Path relativePath;
		IFileSystem* fs = nullptr;

		if (FindMountPoint(name, &relativePath, &fs))
		{
			return fs->FolderExists(relativePath);
		}

		return false;
	}

	bool RootFileSystem::FileExists(const Path& name)
	{
		Path relativePath;
		IFileSystem* fs = nullptr;

		if (FindMountPoint(name, &relativePath, &fs))
		{
			return fs->FileExists(relativePath);
		}

		return false;
	}

	Ref<IBlob> RootFileSystem::ReadFile(const Path& name)
	{
		Path relativePath;
		IFileSystem* fs = nullptr;

		if (FindMountPoint(name, &relativePath, &fs))
		{
			return fs->ReadFile(relativePath);
		}

		return nullptr;
	}

	bool RootFileSystem::WriteFile(const Path& name, const void* data, size_t size)
	{
		Path relativePath;
		IFileSystem* fs = nullptr;

		if (FindMountPoint(name, &relativePath, &fs))
		{
			return fs->WriteFile(relativePath, data, size);
		}

		return false;
	}

	int RootFileSystem::EnumerateFiles(const Path& path, const std::vector<std::string>& extensions, EnumerateCallbackFunc callback, bool allowDuplicates)
	{
		Path relativePath;
		IFileSystem* fs = nullptr;

		if (FindMountPoint(path, &relativePath, &fs))
		{
			return fs->EnumerateFiles(relativePath, extensions, callback, allowDuplicates);
		}

		return Status::PathNotFound;
	}

	int RootFileSystem::EnumerateDirectories(const Path& path, EnumerateCallbackFunc callback, bool allowDuplicates)
	{
		Path relativePath;
		IFileSystem* fs = nullptr;

		if (FindMountPoint(path, &relativePath, &fs))
		{
			return fs->EnumerateDirectories(relativePath, callback, allowDuplicates);
		}

		return Status::PathNotFound;
	}

	static void appendPatternToRegex(const std::string& pattern, std::stringstream& regex)
	{
		for (char c : pattern)
		{
			switch (c)
			{
			case '?': regex << "[^/]?"; break;
			case '*': regex << "[^/]+"; break;
			case '.': regex << "\\."; break;
			default: regex << c;
			}
		}
	}

	String GetFileSearchRegex(const Path& path, const std::vector<std::string>& extensions)
	{
		Path normalizedPath = path.lexically_normal();
		std::string normalizedPathStr = normalizedPath.generic_string();

		std::stringstream regex;
		appendPatternToRegex(normalizedPathStr, regex);
		if (!StringUtils::EndsWith(normalizedPathStr, "/") && !normalizedPath.empty())
			regex << '/';
		regex << "[^/]+";

		if (!extensions.empty())
		{
			regex << '(';
			bool first = true;
			for (const auto& ext : extensions)
			{
				if (!first) regex << '|';
				appendPatternToRegex(ext, regex);
				first = false;
			}
			regex << ')';
		}

		return regex.str();
	}
}
