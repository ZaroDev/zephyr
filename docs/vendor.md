# Vendor Dependencies

Third-party code lives in two places: `Engine/Vendor/` (used by the `Engine` static library) and
`Editor/Vendor/` (used only by the `Editor` application, on top of everything Engine already
pulls in). A separate top-level `Vendor/` directory also exists but is unrelated to engine/editor
dependencies — it holds prebuilt tooling (`Vendor/Binaries/Premake/...` for the premake5
executables used by `Scripts/Setup-*`, and a `Vendor/Binaries/Optick/LICENSE` for the Optick
profiler, which is not currently referenced by any `premake5.lua` or `#include` in the tree).

Two build methods are used:

- **Premake subproject** — the dependency has its own `premake5.lua` and is compiled from source
  as part of the workspace (`include`d from `Build.lua`, built as its own `StaticLib` project,
  linked into `Engine`/`Editor` by project name).
- **Prebuilt library** — the dependency ships headers plus a precompiled `.lib`/`.dll` per
  configuration (`Debug`/`Release`/`Dist`), referenced via the `IncludeDir`/`LibraryDir`/`Library`
  tables in `Dependencies.lua` and linked by path rather than by project name.

Four of the `Engine`/`Editor` dependencies are git submodules (declared in `.gitmodules`): GLFW,
ImGui, ImGuizmo, and nvrhi. The rest (glad, spdlog, assimp, glm, meshoptimizer,
IconFontCppHeaders, and now enkiTS) are vendored as plain committed/copied source or binary
drops, not submodules.

## Engine/Vendor

| Library | Path | Purpose | Build method | Version / notes |
|---|---|---|---|---|
| GLFW | `Engine/Vendor/GLFW` | Windowing, input, and OS surface creation for the renderer. | Premake subproject (`project "GLFW"`, StaticLib), git submodule (fork `ZaroDev/glfw`, pinned at `3.2.1-1355-g026a148d`). | Builds `context.c`, `init.c`, `input.c`, `monitor.c`, `platform.c`, `vulkan.c`, `window.c`, plus a `null_*` backend, and per-OS backend files (`win32_*`/`wgl_*`/`egl_*` on Windows, `x11_*`/`glx_*` on Linux, `cocoa_*`/`nsgl_*` on macOS). Not the stock upstream GLFW — a fork under the `ZaroDev` GitHub account. |
| glad | `Engine/Vendor/glad` | OpenGL function-pointer loader. | Premake subproject (`project "glad"`, StaticLib, C not C++). | Builds only `src/glad.c` plus the `glad.h`/`khrplatform.h` headers — a minimal, generated loader; the specific GL version/profile it was generated for isn't recorded in the vendored files themselves. Present in the build graph even though the engine's active graphics backends (per `Dependencies.lua`/nvrhi) are Vulkan and D3D11/D3D12 — likely a holdover from an earlier OpenGL-based renderer, or reserved for a future GL backend. |
| ImGui | `Engine/Vendor/ImGui` | Immediate-mode GUI, used by the Editor (and any in-engine debug UI, e.g. the CVar editor). | Premake subproject (`project "ImGui"`, StaticLib), git submodule (fork `ZaroDev/imgui`, `docking` branch, pinned at `v1.91.9b-docking-13-gd5b98cb19`). | Builds core ImGui sources plus `misc/cpp/imgui_stdlib.*` and `imgui_demo.cpp`. Shows as untracked (`??`) in `git status` despite being a valid, pinned submodule — see the "Known inconsistencies" note in `docs/build-system.md` (stale `.gitmodules` section name `Editor/Vendor/ImGui` left over from before this vendor tree was reorganized under `Engine/`). Note the docking branch is required for the Editor's dockspace-based panel layout. |
| assimp | `Engine/Vendor/assimp` | Model/scene import (glTF, FBX, OBJ, etc.) for mesh loading. | Prebuilt library. `Dependencies.lua` points `IncludeDir["assimp"]` at `include/` and `Library["assimp"]` at `lib/%{cfg.buildcfg}/assimp.lib`. | Ships prebuilt `Debug`/`Release`/`Dist` lib directories. No version macros found in the vendored `include/assimp/version.h` (it only defines the include guard) — version not pinned/undetermined from the repo. `Engine/Build-Engine.lua` copies `assimp.dll` to `Assets/` as a postbuild step, so this is a dynamically-linked dependency at runtime, not just link-time. |
| glm | `Engine/Vendor/glm` | Header-only math library (vectors, matrices, quaternions) used throughout `Engine/src/Zephyr/Math`. | Header-only, not a premake subproject — just an include path (`IncludeDir["glm"]`). | Version macro found: `GLM_VERSION_MESSAGE "GLM: version 1.0.0"` — pinned at **1.0.0**. |
| meshoptimizer | `Engine/Vendor/meshoptimizer` | Mesh optimization (vertex cache/overdraw/fetch optimization, simplification) for imported geometry. | Prebuilt library, same pattern as assimp: `Library["meshoptimizer"]` points at `lib/%{cfg.buildcfg}/meshoptimizer.lib`. | Ships `Debug`/`Release`/`Dist` prebuilt libs and a single `meshoptimizer.h` header. Version not pinned/undetermined from the repo (no version macro checked in the header at a glance; upstream ships one but it wasn't confirmed present here). |
| spdlog | `Engine/Vendor/spdlog` | Logging backend for `Core/Log.h`. | Header-only-style vendoring — no `premake5.lua` of its own; just an include path (`IncludeDir["spdlog"]`), compiled as part of whatever includes it. | Version macros found in `include/spdlog/version.h`: `SPDLOG_VER_MAJOR 1`, `MINOR 10`, `PATCH 0` — pinned at **1.10.0**. |
| NVRHI (Vulkan/D3D11/D3D12) | `Engine/Vendor/nvrhi` (referenced from `Build.lua` as `Engine/Vendor/NVRHI` — casing mismatch, see `docs/build-system.md`) | Render Hardware Interface abstraction layer — the engine's graphics API abstraction, wrapping Vulkan and D3D11/D3D12. | Premake subproject, four separate `StaticLib` projects in one `premake5.lua`: `NVRHI-Vulkan`, `NVRHI-D3D11`, `NVRHI-D3D12`, and an umbrella `NVRHI` project that `links` all three. Git submodule (fork `ZaroDev/nvrhi`, tracking `heads/main`, currently 1 commit ahead of the pinned/checked-out commit per `git submodule status` showing a `+` prefix — i.e. locally modified/advanced relative to what's recorded in the index). | Also bundles `rtxmu` (NVIDIA's ray-tracing acceleration-structure management library) as a sub-component (`NVRHI_WITH_RTXMU=1` define, `rtxmu/` sources compiled into the Vulkan/D3D12/umbrella projects). `NVRHI-D3D12` additionally expects `thirdparty/DirectX-Headers/include`. Engine links the umbrella `nvrhi` project plus a separately prebuilt `slang.lib` (Slang shader compiler/reflection, pulled from the Vulkan SDK's `Lib` directory via `Library["slang"]` in `Dependencies.lua` — not built from source). |
| enkiTS | `Engine/Vendor/enkiTS` | "enki Task Scheduler" — a lightweight C++11 task scheduler for data/task-parallel work on multicore CPUs (used in enkisoftware's Avoyd; upstream by dougbinks). | **Vendored only — not wired into the build.** No `premake5.lua` of its own, and it is not referenced anywhere in `Build.lua`, `Dependencies.lua`, or `Engine/Build-Engine.lua` (confirmed by grep). It also does not appear as an `IncludeDir` entry, so nothing currently `#include`s it either. | Source drop is minimal: `src/TaskScheduler.h`, `src/TaskScheduler.cpp`, `src/LockLessMultiReadPipe.h` (only the C++ API subset — no `TaskScheduler_c.*`, so the C API wasn't included). No version/commit is pinned; it's a plain directory, not a submodule, and shows as untracked (`??`) in `git status` — i.e. added to the working tree but not yet committed. To actually use it, the Engine build would need either a new `include "Engine/Vendor/enkiTS"` premake subproject (upstream suggests just compiling `TaskScheduler.cpp` directly, so a small custom `premake5.lua` or direct inclusion in `Engine/Build-Engine.lua`'s `includedirs`/`files` would work) plus wiring `#include "TaskScheduler.h"` into engine code. |

## Editor/Vendor

| Library | Path | Purpose | Build method | Version / notes |
|---|---|---|---|---|
| ImGuizmo | `Editor/Vendor/ImGuizmo` | In-viewport 3D gizmo widgets (translate/rotate/scale manipulators) for the Editor's scene viewport. | Compiled directly into the `Editor` project — `Editor/Build-Editor.lua`'s `files {}` explicitly lists `Vendor/ImGuizmo/ImGuizmo.h`/`.cpp` (not a separate premake subproject/StaticLib). Git submodule (upstream `CedricGuillemet/ImGuizmo`, pinned at `1.83-83-gba662b1`). | Only `ImGuizmo.h`/`.cpp` are compiled in, even though the vendored directory also contains `GraphEditor`, `ImCurveEdit`, `ImGradient`, and `ImSequencer` sources — those extra widgets are present on disk but unused/uncompiled. |
| IconFontCppHeaders | `Editor/Vendor/IconFontCppHeaders` | Generated C++ header constants for icon font codepoints (e.g. FontAwesome), used for icon glyphs in the Editor UI. | Header-only — just an include path (`IconHeaders` in `Dependencies.lua`), not a premake subproject and not a git submodule (plain vendored drop; not listed in `.gitmodules`). | Contains a `FontIcons/` subdirectory of generated headers; no version pinned in-repo. |

## enkiTS integration status (explicit summary)

enkiTS is **vendored but not integrated**: the source is present at `Engine/Vendor/enkiTS/`
(untracked in git — not yet committed), but:
- it has no `premake5.lua` and is not `include`d from `Build.lua`,
- it is not listed in `Dependencies.lua`'s `IncludeDir` table,
- it is not in `Engine/Build-Engine.lua`'s `includedirs`/`links`, and
- a grep of `Engine/src/` for `enki` (both `#include <enki...` and `enki::`) found zero matches.

In short, it's sitting in the vendor tree ready to be wired up, but nothing in the engine
currently depends on or uses it.
