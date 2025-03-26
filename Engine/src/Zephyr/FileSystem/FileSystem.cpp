#include <pch.h>
#include "FileSystem.h"

#include <fstream>

namespace Zephyr
{
	namespace FileSystem
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
		Path ReplaceExtension(const Path& path, StrView extension)
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
		String ReadFileContents(const Path& path)
		{
			File file(path, File::INPUT);
			StringStream stream = file.ReadStream();
			return stream.str();
		}
	}

	File::File(const Path& filePath, i32 mode)
	{
		m_Stream.open(filePath, mode);
	}

	File::File(const Path& filePath, OpenMode mode)
	{
		m_Stream.open(filePath, mode);
	}
	File::~File()
	{
		m_Stream.close();
	}
	bool File::GetLine(String& string)
	{
		return static_cast<bool>(std::getline(m_Stream, string));
	}
	void File::Read(Buffer& buffer)
	{
		m_Stream.read((char*)buffer.Data, buffer.Size);
	}
	StringStream File::ReadStream() const
	{
		StringStream stream;
		stream << m_Stream.rdbuf();
		return stream;
	}
	void File::Write(const Buffer& buffer)
	{
		//m_Stream.write((const char*)buffer.Data, buffer.Size);
	}
	bool File::IsEndOfFile() const
	{
		return m_Stream.eof();
	}
	size File::Size()
	{
		size start = m_Stream.tellg();
		m_Stream.seekg(0, std::ios::end);
		size end = m_Stream.tellg();
		m_Stream.seekg(0, std::ios::beg);

		return end - start;
	}
}
