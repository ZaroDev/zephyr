#pragma once
#include "Buffer.h"

namespace Zephyr
{
	class File final 
	{
	public:
		enum OpenMode : u8
		{
			INPUT = 0x01,
			OUTPUT = 0x02,
			ATE = 0x04,
			APP = 0x08,
			TRUNC = 0x10,
			BINARY = 0x20,
		};

		
		explicit File(const Path& filePath, i32 mode);
		explicit File(const Path& filePath, OpenMode mode);
		~File();

		DEFAULT_MOVE_AND_COPY(File);

		bool GetLine(String& string);
		void Read(Buffer& buffer);
		StringStream ReadStream() const;
		void Write(const Buffer& buffer);
		bool IsEndOfFile() const;
		size Size();

		/*File& operator<<(const String& string)
		{
			m_Stream << string;
			return *this;
		}

		File& operator<<(StrView string)
		{
			m_Stream << string;
			return *this;
		}

		File& operator<<(char ch)
		{
			m_Stream << ch;
			return *this;
		}*/
		
		void operator>>(String& string)
		{
			m_Stream >> string;
		}

		bool IsOpen() const
		{
			return m_Stream.is_open();
		}

	private:
		FileStream m_Stream;
	};

	namespace FileSystem
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

		Path ReplaceExtension(const Path& path, StrView extension);

		Path GetFileName(const Path& path);

		bool IsDirectory(const Path& path);

		String ReadFileContents(const Path& path);
		Ref<Buffer> ReadFile(const Path& path);
	}
}