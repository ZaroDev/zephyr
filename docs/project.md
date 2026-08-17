# Project

The `Project` submodule is meant to represent an on-disk Zephyr project — its name, its working directory, its start scene, and its asset registry location — as a singleton "active project" that the rest of the engine (asset browser, scene loading, etc.) can query. `ProjectSerializer` is meant to handle reading/writing that project data to disk. In its current state, however, this submodule is largely a stub: the data model and the "active project" access pattern are in place, but loading, saving, and serialization are not implemented.

## Key Files

| File | Purpose |
|---|---|
| `Project.h` | Declares `ProjectData` (the on-disk project fields) and `Project` (the active-project singleton + accessors). |
| `Project.cpp` | Implements `Project::New/Load/Save` — currently a stub (see Design Notes). |
| `ProjectSerializer.h` | Empty — declares the `Zephyr` namespace and nothing else. |
| `ProjectSerializer.cpp` | Empty — same. |

## Key Types & APIs

- **`ProjectData`** — a plain struct holding `Name` (default `"Untitled"`), `StartScene`, `WorkingDirectory`, and `AssetRegistryPath` (the latter three all `Zephyr::Path`). This is the intended serializable payload of a `.zproj`-style project file, though no file extension, schema, or serializer exists yet to give that shape meaning.

- **`Project`** — holds one `ProjectData m_Data` and one `Path m_ProjectPath` (the directory the project file lives in), plus a static `s_ActiveProject` (`inline static Ref<Project>`) representing "the currently open project" as a process-wide singleton.
  - `GetActive()` — returns the current `Ref<Project>` (may be null if no project has been created/loaded).
  - `GetProjectPath()` (static) / `GetProjectDirectory()` (instance) — both return `m_ProjectPath`; `GetProjectPath()` asserts `s_ActiveProject` is set and reads from it directly, while `GetProjectDirectory()` is a plain instance accessor with no such guard (call it through a valid `Ref<Project>`, not through `GetActive()` if that might be null — see Design Notes). Callers in the tree only use the static accessors.
  - `GetWorkingDirectory()` / `GetAssetRegistryPath()` (both static) — return `GetProjectPath() / m_Data.WorkingDirectory` / `.../m_Data.AssetRegistryPath` respectively, i.e. those fields are stored relative to the project directory and resolved to absolute paths on access.
  - `GetData()` — mutable access to `m_Data` (instance method).
  - `New()` — creates a fresh `Project`, sets it as `s_ActiveProject`, and returns it. **Does not set `m_ProjectPath`** or populate `m_Data` beyond its in-class defaults (see Design Notes).
  - `Load(const Path& path)` — declared to load a project from disk; **currently returns a default-constructed (null) `Ref<Project>` unconditionally** — not implemented.
  - `Save(const Path& path)` — declared to persist the active project to disk; **currently returns `false` unconditionally** — not implemented.

- **`ProjectSerializer`** — the header/source pair exist but contain no declarations or definitions at all (just an empty `namespace Zephyr {}`). This is where `Project::Load`/`Save` would presumably delegate to once implemented, but as of now there is nothing to call.

## Design Notes

- **`Load` and `Save` are unimplemented stubs.** `Project::Load` always returns `nullptr` (a default `Ref<Project>`) regardless of the path passed in, and `Project::Save` always returns `false`. Neither touches the filesystem, `IFileSystem`, or `ProjectSerializer` in any way. Any caller currently invoking these would silently fail to load/save a project — there are no callers in the current tree, so this hasn't surfaced as a visible bug yet, but the API surface is misleading as-is (it looks functional from the header).
- **No serialization format decided/implemented.** `ProjectSerializer.h/.cpp` are completely empty. There's no `yaml-cpp` or similar text-serialization vendor library present under `Engine/Vendor/` to infer an intended format from (many engines with this file-naming convention use YAML, but nothing in this codebase confirms that choice — it should not be assumed). Treat the on-disk project file format as entirely undecided at this point.
- **`Project::New()` never sets `m_ProjectPath`.** It default-constructs `Project` (so `m_Data` gets its in-class defaults: `Name = "Untitled"`, empty paths) and assigns it to `s_ActiveProject`, but never assigns `m_ProjectPath`. `GetProjectPath()`/`GetWorkingDirectory()`/`GetAssetRegistryPath()` would therefore resolve against a default-constructed (empty) `Path` until something else sets it — and nothing in the current code ever does (there's no setter for `m_ProjectPath` at all). This looks like an unfinished part of project creation: presumably `New()` is meant to take a target directory/name argument, or a setter needs to be added.
- **`GetActive()` can be null; static accessors are called through it anyway.** `Editor/src/Panels/AssetBrowserPanel.cpp` calls `Zephyr::Project::GetActive()->GetWorkingDirectory()`. Because `GetWorkingDirectory()` is `static`, calling it via `->` on a (potentially null) `Ref<Project>` doesn't actually dereference the pointer — it's legal and safe even if `GetActive()` returns null, and the method's own `CORE_ASSERT(s_ActiveProject)` is what would catch the "no project" case. This is a subtle bit of C++ (static-method-via-instance-pointer) that's easy to misread as a null-deref risk; it isn't, but it does mean the assert only fires if `s_ActiveProject` itself is unset, not based on whatever `GetActive()` happened to return at the call site.
- **No unload/close.** There's no `Project::Close()` or equivalent to clear `s_ActiveProject`; once a project is created via `New()`, the "active project" singleton is never explicitly reset in this submodule.
- **Ownership/lifetime.** `Project` is reference-counted (`Ref<Project>` = `std::shared_ptr<Project>`), and the sole owning reference the engine keeps by default is the static `s_ActiveProject`. Any caller holding a separate `Ref<Project>` from `New()`/`Load()`/`GetActive()` shares ownership; there's no notion of "the" project being torn down deterministically since it's shared_ptr-managed.

## Dependencies

- **Depends on:** `Zephyr/Core/Base.h` (`Path`, `String`, `Ref`/`CreateRef`, `CORE_ASSERT`). `Project.h` is included by the engine's umbrella header `Zephyr.h`, so it's implicitly available anywhere the engine header is included.
- **Depended on by:** `Editor/src/Panels/AssetBrowserPanel.cpp` calls `Zephyr::Project::GetActive()->GetWorkingDirectory()` to determine the asset root to browse and to detect when the active project's working directory has changed. No other current call sites were found for `Project` or `ProjectSerializer` in `Engine` or `Editor`.
