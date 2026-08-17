# Zephyr Engine Documentation

Zephyr is a C++20 game engine built around a static-library `Engine` and a single executable, `Editor`, which links it. There is currently no separate runtime/player target — `Editor` is the only thing that gets built and run. The project is built with [Premake5](https://premake.github.io/) and targets Windows first (with a Linux generator script present but the Vulkan/D3D11 split assumes Windows as primary).

This documentation reflects the working tree on the `engine-overhaul` branch as of 2026-08-17, which is an **active, mid-flight rewrite** of the rendering layer. Several modules described here are unfinished, dead, or temporarily disconnected from the runtime — each doc calls this out explicitly rather than describing an idealized design. Treat this as a snapshot of a moving target, not a finished architecture.

## How the pieces fit together

```
                          ┌─────────────────────────┐
                          │         Editor           │  Editor/src/App.cpp
                          │  (the only executable)   │  links against Engine
                          └────────────┬─────────────┘
                                       │ owns Panels, a Scene, a Project
                                       ▼
                          ┌─────────────────────────┐
                          │  Zephyr::Application      │  Core/Application.*
                          │  (Engine's base class)    │  main loop: Update → Render
                          └────────────┬─────────────┘
                                       │ registers & ticks
                                       ▼
                    ┌──────────────────────────────────────┐
                    │            ModuleManager               │  Modules/
                    │  Window → Input → Renderer (IModule)   │
                    └───────┬───────────────┬────────────────┘
                            │               │
                            ▼               ▼
                      Window (GLFW)   Renderer (IModule) ──creates──> RHI::Device
                                                                       (nvrhi, Vulkan only,
                                                                        mostly stubbed)

              Also present, independent of Application/ModuleManager:
              ECS::Scene/Entity  ·  Project  ·  FileSystem  ·  Math  ·  Time  ·  CVarManager

              Orphaned but still compiled, unreachable from any live code path:
              Renderer::DeviceManager (+ D3D11/Vulkan subclasses) · IRenderPass · ImGuiRenderPass
```

## Module reference

| Area | Doc | Summary |
|---|---|---|
| **Core** | [core.md](core.md) | `Application` lifecycle, logging, asserts, CVars, UUIDs, allocation tracking — the foundation everything else builds on |
| **ECS** | [ecs.md](ecs.md) | `entt`-backed `Scene`/`Entity`/`Components` |
| **Math** | [math.md](math.md) | `glm`-aliasing vector/matrix/quaternion types and transform helpers |
| **Time** | [time.md](time.md) | Frame timing — **note:** `GetDeltaTime()` returns milliseconds despite the name |
| **Utils** | [utils.md](utils.md) | Small standalone helpers (`StringHash`, `BytesToMB`, etc.) |
| **FileSystem** | [filesystem.md](filesystem.md) | Virtual filesystem layer, `Buffer`/`Blob`, Windows file dialogs |
| **Input** | [input.md](input.md) | GLFW-polling input module (keyboard/mouse), mid-migration to `IModule` |
| **Window** | [window.md](window.md) | GLFW window/lifecycle owner, ticks as an `IModule` |
| **Modules** | [modules.md](modules.md) | The `IModule`/`ModuleManager` plugin-and-tick system underpinning Window/Input/Renderer |
| **Project** | [project.md](project.md) | Active-project singleton — **load/save are unimplemented stubs**, no serialization format chosen yet |
| **RHI** | [rhi.md](rhi.md) | New low-level nvrhi device abstraction — **skeleton only**, Vulkan-only, mostly stub bodies |
| **Renderer** | [renderer.md](renderer.md) | Both the new `Renderer` `IModule` and the old, fully-working-but-orphaned `DeviceManager`/`IRenderPass`/ImGui-integration stack |
| **Editor** | [editor.md](editor.md) | The `App` and its Panels (Asset Browser, Console, Hierarchy, Info, Project, Scene) |
| **Build System** | [build-system.md](build-system.md) | Premake5 workspace/project layout, configs, how to generate/build |
| **Vendor** | [vendor.md](vendor.md) | Third-party libraries: what's used, what's vendored-but-inert |

## Current state: what actually runs today

The individual docs go into detail, but pulled together, the branch is in a state where **the editor UI does not render and the new render path does not draw anything**:

1. **No panel UI renders.** `Zephyr::Application::Render()` has its `OnImGui(deltaTime)` call commented out (see [core.md](core.md), [editor.md](editor.md)). The Editor's panels are otherwise implemented but never invoked.
2. **Two renderers coexist, and the live one doesn't work yet.** `Renderer/DeviceManager` + `IRenderPass` + `ImGuiRenderPass` is a complete, working Vulkan/D3D11 stack ported from NVIDIA's donut framework — but nothing outside `Renderer/` instantiates it, so it's dead code. The new `Renderer` `IModule` creates an `RHI::Device` instead, but `RHI::Device`/`RHI::VulkanDevice` are largely stub method bodies with no swapchain or draw path, and `RHI::Device::Create()` doesn't even return a value on the D3D11/D3D12 branches. See [rhi.md](rhi.md) and [renderer.md](renderer.md) for the full breakdown.
3. **Physics tick phase is fully plumbed but never invoked** from `Application::Run()` ([core.md](core.md), [modules.md](modules.md)).
4. **`Project::Load`/`Save` are unconditional stubs**, and `ProjectSerializer.h/.cpp` are empty — there's no on-disk project format yet ([project.md](project.md)).
5. **`ScenePanel` is dead in practice** (viewport never assigned) and **`ProjectPanel` is an empty stub** ([editor.md](editor.md)).
6. **`enkiTS` is vendored but not wired into the build at all** — not in Premake, zero usages in source ([vendor.md](vendor.md)).

None of this is presented as broken-and-unknown to whoever picks this branch back up — it's the expected shape of an in-progress rewrite. The point of listing it here is so the next person (human or agent) doesn't have to rediscover it from scratch.

## Known bugs and inconsistencies worth fixing

Collected from all module docs — see each doc's "Design Notes" for full context:

- `Time::GetDeltaTime()` returns **milliseconds**, not seconds, despite the name and despite `GetTimeSinceStart()` accumulating in seconds in the same file.
- CVar system only has a working implementation for `bool`; int/float/string CVars null-dereference.
- Inverted assert condition in `ApplicationCommandLineArgs::operator[]`.
- `Blob` (FileSystem) allocates with `malloc` but frees with `delete`.
- `IOStream.h` references an undefined `Size` type — dead code, currently uncompiled since nothing includes the header.
- `Build.lua` includes `Engine/Vendor/NVRHI` (capital) while the real directory is lowercase `nvrhi` — silently works only because of case-insensitive filesystems/git config; would break on Linux.
- `.gitmodules` still names the ImGui submodule section `Editor/Vendor/ImGui` after the vendor tree moved to `Engine/Vendor/ImGui` — cosmetic, but confusing (`git status` shows it as untracked-looking despite being a pinned submodule).
- `HierarchyPanel::DrawComponents` mislabels a `MeshComponent` UI header as "Transform" (copy-paste artifact).
- `Renderer/GraphicsAPI.h/.cpp` → `RHI/GraphicsAPI.h/.cpp` migration dropped the `ToNVRHI()` conversion helper, which is now unreferenced but will likely be needed again once `RHI::Device` has to interoperate with `nvrhi::GraphicsAPI`-speaking code like `ShaderFactory`.

## Building

See [build-system.md](build-system.md) for full detail. Short version: run the generator script under `Scripts/` for your platform (`Setup-Windows.bat` → `premake5.exe --file=Build.lua vs2022`, or `Setup-Linux.sh` → gmake2), then build the generated project/solution. The `VULKAN_SDK` environment variable must be set — `Dependencies.lua` reads it directly for Vulkan SDK includes/libs and the vendored `slang` compiler library.
