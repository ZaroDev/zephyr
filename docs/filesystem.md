# FileSystem

The `FileSystem` submodule is Zephyr's file I/O layer. It provides a small virtual-filesystem (VFS) abstraction for reading and writing files/directories through mountable backends, a raw-memory `Buffer` helper for owning binary blobs, a (currently unused) binary stream reader/writer, and OS-level file dialogs for the editor's "Open"/"Save" workflows. It exists to decouple the rest of the engine from `std::filesystem`/WinAPI calls and to give tools (the Editor, asset pipeline) a uniform way to address files regardless of where they physically live on disk.

## Key Files

| File | Purpose |
|---|---|
| `Buffer.h` | A lightweight, manually-managed (`malloc`/`free`) owning byte buffer, `Buffer`. |
| `FileSystem.h` / `.cpp` | The VFS interfaces and implementations: `IBlob`/`Blob`, `IFileSystem`, `DefaultFileSystem`, `RelativeFileSystem`, `RootFileSystem`. |
| `IOStream.h` | `BlobStreamReader` / `BlobStreamWriter` — cursor-based binary readers/writers over a raw buffer. |
| `FileDialogs.h` / `.cpp` | Platform-agnostic `OpenFile`/`SaveFile` entry points that dispatch to a platform backend. |
| `Platform/Windows/WindowsPlatformUtils.h` / `.cpp` | Windows implementation of the file dialogs, using the classic `GetOpenFileNameA`/`GetSaveFileNameA` common dialog API. |

## Key Types & APIs

- **`Buffer`** (`Buffer.h`) — a POD-like struct wrapping a `u8* Data` and `u64 Size`. `Allocate`/`Release` manage memory with `malloc`/`free`; `Copy` deep-copies another `Buffer`. It has no destructor, so `Release()` must be called explicitly or the memory leaks — it is a manual-lifetime type, not RAII. `operator bool` lets you test "does this buffer hold data".

- **`IBlob` / `Blob`** (`FileSystem.h`) — `IBlob` is a read-only interface over an opaque byte range (`Data()`/`Size()`), returned by file reads. `Blob` is the concrete, owning implementation; it's constructed with a raw pointer + size and frees the memory in its destructor. Note: `Blob`'s data is allocated with `std::malloc` in `DefaultFileSystem::ReadFile` but freed with `delete` in `Blob::~Blob()` (see Design Notes — this is a real mismatch).

- **`IFileSystem`** — the VFS interface: `FolderExists`, `FileExists`, `ReadFile` (-> `Ref<IBlob>`), `WriteFile`, `EnumerateFiles`, `EnumerateDirectories`. All paths are `Zephyr::Path` (`std::filesystem::path`).
  - **`DefaultFileSystem`** maps directly onto the OS filesystem via `std::ifstream`/`std::ofstream` and, for enumeration, native Win32 `FindFirstFileA`/`FindNextFileA` (with a `glob64`-based POSIX fallback behind `#else`, untested/unused on this Windows-only branch).
  - **`RelativeFileSystem`** wraps another `IFileSystem` and prefixes every request with a fixed base path — effectively a "chroot" view.
  - **`RootFileSystem`** is a composite VFS with no backing store of its own: you `Mount(virtualPath, someFS)` or `Mount(virtualPath, nativePath)` (a convenience overload that builds a `RelativeFileSystem` over a `DefaultFileSystem`), and all calls are routed to the deepest matching mount point via `FindMountPoint`. This is the type intended to sit at the root of the engine's asset/file access (e.g. `assets://`, `project://`-style virtual roots), though nothing in the current tree actually constructs or uses a `RootFileSystem`, `DefaultFileSystem`, or `RelativeFileSystem` yet — the VFS layer appears to be scaffolding ahead of its consumers.

- **`GetFileSearchRegex`** — builds a regex string from a glob-like path pattern (`*`, `?`) plus a list of extensions; used internally for search UIs (not currently called anywhere else in the tree).

- **`BlobStreamReader` / `BlobStreamWriter`** (`IOStream.h`) — cursor-based helpers for reading/writing primitive values and raw byte ranges sequentially over a caller-owned buffer, with `CORE_ASSERT`-based bounds checks on write. Non-copyable/non-movable by design (`DISABLE_MOVE_AND_COPY`).

- **`FileDialogs::OpenFile(StrView filter)` / `SaveFile(StrView filter)`** (`FileDialogs.h`) — the only entry points application code should call. They `#ifdef PLATFORM_WINDOWS` dispatch to `FileDialogs::Windows::OpenFile/SaveFile`; any other platform is a hard `#error "Not implemented for the current platform"` at compile time.

- **`FileDialogs::Windows::OpenFile/SaveFile`** (`WindowsPlatformUtils.cpp`) — thin wrappers around `GetOpenFileNameA`/`GetSaveFileNameA`. They fetch the native `HWND` via `glfwGetWin32Window(Application::Get().GetModule<Window>()->GetGLFWHandle())`, i.e. they reach into the engine's module system to find the `Window` module and pull its GLFW handle for dialog ownership.

## Design Notes

- **Windows-only.** Everything under `Platform/Windows/` is guarded by `PLATFORM_WINDOWS`; the file-dialog feature has no other platform implementation, so `FileDialogs.cpp` will fail to compile on any non-Windows target (`#error`). The VFS types in `FileSystem.cpp` have a POSIX `glob64` fallback for enumeration, but it's unexercised on this branch.
- **`malloc`/`delete` mismatch.** `DefaultFileSystem::ReadFile` allocates the file contents with `std::malloc`, but `Blob::~Blob()` frees it with `delete`. Mixing allocators like this is undefined behavior in the general case; this looks like a bug introduced when `Blob`'s implementation and its only real producer (`DefaultFileSystem::ReadFile`) were written independently.
- **`Buffer` has no destructor.** Unlike `Blob`, `Buffer` is a manual-lifetime value type — callers must remember to call `Release()`. There's no RAII guard, so it's easy to leak; treat it as a "you asked for it, you free it" primitive rather than a smart owner.
- **VFS is currently unwired.** No code in `Engine` or `Editor` constructs a `RootFileSystem`, `DefaultFileSystem`, or `RelativeFileSystem`, and `GetFileSearchRegex` has no callers either. The interface/implementation is complete and self-consistent, but it isn't yet plugged into asset loading, project loading, or anything else — read this as forward-looking infrastructure, not something exercised by any current code path.
- **`IOStream.h` likely doesn't compile in isolation.** `BlobStreamReader`/`BlobStreamWriter` use a bare type named `Size` (e.g. `void Read(u8* buffer, Size length)`), but no `Size` type is defined anywhere in `Engine/src` (only `SizeT` exists, in `BasicTypes.h`). Nothing in the current tree `#include`s `IOStream.h`, so this has evidently never been compiled since the type was renamed/introduced — it's dead code in its current state.
- **Fragile `StrView` filter for WinAPI.** `FileDialogs::OpenFile`/`SaveFile` take a `Zephyr::StrView` (`std::string_view`) and pass `filter.data()` straight into `OPENFILENAMEA::lpstrFilter`, which WinAPI expects to be a null-terminated (and internally double-null-terminated, multi-part) C string. `string_view` makes no such guarantee; this only works because callers currently pass in string literals. Passing a non-null-terminated `string_view` (e.g. a substring) would read out of bounds.
- **`SaveFile` reuses `OpenFile`'s flags.** `Windows::SaveFile` sets `OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST`, which requires the target file to already exist — the opposite of what a typical "Save As" dialog wants (creating a new file). This looks like a copy-paste artifact from `OpenFile` rather than intentional behavior.
- **Recently migrated to the module system.** `WindowsPlatformUtils.cpp` used to fetch the GLFW window via `Application::Get().GetDeviceManager().GetWindow()`; it was just changed (uncommitted on this branch) to `Application::Get().GetModule<Window>()->GetGLFWHandle()`, following the broader `DeviceManager` → `IModule`/`Window` refactor happening across the engine right now.

## Dependencies

- **Depends on:** `Zephyr/Core` (`Base.h` for `Path`/`String`/`Ref`/`Scope`, `BasicTypes.h`, `Assert.h`), `Zephyr/Window` (for `Window::GetGLFWHandle()`, Windows dialogs only), `Zephyr/Core/Application` (`Application::Get().GetModule<Window>()`), GLFW (`glfwGetWin32Window` via `glfw3native.h`), and the Windows SDK (`commdlg.h`) on the Windows path.
- **Depended on by:** the Editor's file-open/save flows go through `Zephyr::FileDialogs::OpenFile/SaveFile` (used by Editor panels such as the asset browser / project tooling). `Buffer` and the `IBlob`/`Blob` types are general-purpose and intended for use by asset loading code elsewhere in the engine, though no current file reads through `DefaultFileSystem` yet.
