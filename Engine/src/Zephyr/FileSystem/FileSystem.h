/*
MIT License

Copyright (c) 2025 ZaroDev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
#include <Zephyr/Core/Base.h>

namespace Zephyr
{
	namespace Status
	{
		constexpr i32 OK = 0;
		constexpr i32 Failed = -1;
		constexpr i32 PathNotFound = -2;
		constexpr i32 NotImplemented = -3;
	}

	/*
	 * @interface IBlob
	 * @brief Base class for untyped data, typically read from a file.
	 */
	class IBlob
	{
	public:
		virtual ~IBlob() = default;

		// Getter for the stored data
		NODISCARD virtual const void* Data() const = 0;

		// Getter for the size of the stored data
		NODISCARD virtual SizeT Size() const = 0;


		// Returns true if the provided blob contains no data.
		static constexpr bool IsEmpty(IBlob const* blob)
		{
			return blob == nullptr || blob->Data() == nullptr || blob->Size() == 0;
		}
	};

	/*
	 * @class Blob
	 * @brief Specific blob implementation that owns the data and frees it when deleted.
	 */
	class Blob final : public IBlob
	{
	public:
		Blob(void* data, SizeT size);
		virtual ~Blob() override;

		NODISCARD const void* Data() const override;
		NODISCARD SizeT Size() const override;

	private:
		void* m_Data;
		SizeT m_Size;
	};

	typedef const std::function<void(StrView)>& EnumerateCallbackFunc;

	/*
	 * @interface IFileSystem
	 * @brief Basic interface for the virtual file system.
	 */
	class IFileSystem
	{
	public:
		virtual ~IFileSystem() = default;

		virtual bool FolderExists(const Path& path) = 0;
		virtual bool FileExists(const Path& path) = 0;

		virtual Ref<IBlob> ReadFile(const Path& path) = 0;
		virtual bool WriteFile(const Path& path, const void* data, SizeT size) = 0;

		virtual i32 EnumerateFiles(const Path& path, const std::vector<String>& extensions, EnumerateCallbackFunc callback, bool allowDuplicates = false) = 0;
		virtual i32 EnumerateDirectories(const Path& path, EnumerateCallbackFunc callback, bool allowDuplicates = false) = 0;
	};

	/*
	 * @class DefaultFileSystem
	 * @brief An implementation of virtual file system that directly maps to the OS files.
	 */
	class DefaultFileSystem final : public IFileSystem
	{
	public:
		DefaultFileSystem() = default;
		virtual ~DefaultFileSystem() override = default;
		virtual bool FolderExists(const Path& path) override;
		virtual bool FileExists(const Path& path) override;
		virtual Ref<IBlob> ReadFile(const Path& path) override;
		virtual bool WriteFile(const Path& path, const void* data, SizeT size) override;
		virtual i32 EnumerateFiles(const Path& path, const std::vector<String>& extensions, EnumerateCallbackFunc callback, bool allowDuplicates) override;
		virtual i32 EnumerateDirectories(const Path& path, EnumerateCallbackFunc callback, bool allowDuplicates) override;
	};

	/*
	 * @class RelativeFileSystem
	 * @brief A layer that represents some path in the underlying file system as an entire FS.
     * Effectively, just prepends the provided base path to every file name
	 * and passes the requests to the underlying FS.
	 */
	class RelativeFileSystem final : public IFileSystem
	{
	public:

		RelativeFileSystem(Scope<IFileSystem> parent, const Path& basePath);
		NODISCARD const Path& GetBasePath() const { return m_BasePath; }

		virtual ~RelativeFileSystem() override = default;
		virtual bool FolderExists(const Path& path) override;
		virtual bool FileExists(const Path& path) override;
		virtual Ref<IBlob> ReadFile(const Path& path) override;
		virtual bool WriteFile(const Path& path, const void* data, SizeT size) override;
		virtual i32 EnumerateFiles(const Path& path, const std::vector<String>& extensions, EnumerateCallbackFunc callback, bool allowDuplicates) override;
		virtual i32 EnumerateDirectories(const Path& path, EnumerateCallbackFunc callback, bool allowDuplicates) override;

	private:
		Scope<IFileSystem> m_UnderlyingFS;
		Path m_BasePath;
	};

	/*
	 * @class RootFileSystem
	 * @brief A virtual file system that allows mounting, or attaching, other VFS objects to paths.
	 * Does not have any file systems by default, all of them must be mounted first.
	 */
	class RootFileSystem final : public IFileSystem
	{
	public:
		virtual ~RootFileSystem() override = default;

		virtual bool FolderExists(const Path& path) override;
		virtual bool FileExists(const Path& path) override;
		virtual Ref<IBlob> ReadFile(const Path& path) override;
		virtual bool WriteFile(const Path& path, const void* data, SizeT size) override;
		virtual i32 EnumerateFiles(const Path& path, const std::vector<String>& extensions, EnumerateCallbackFunc callback, bool allowDuplicates) override;
		virtual i32 EnumerateDirectories(const Path& path, EnumerateCallbackFunc callback, bool allowDuplicates) override;

		void Mount(const Path& path, Ref<IFileSystem> fs);
		void Mount(const Path& path, const Path& nativePath);
		bool UnMount(const Path& path);
	private:
		bool FindMountPoint(const Path& path, Path* pRelativePath, IFileSystem** ppFS);
	private:
		std::vector <std::pair<String, Ref<IFileSystem>>> m_MountingPoints;
	};

	String GetFileSearchRegex(const Path& path, const std::vector<String>& extensions);
}
