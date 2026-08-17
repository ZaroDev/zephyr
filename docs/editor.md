# Editor

The Editor is Zephyr's ImGui-based tooling application: a `ConsoleApp` (`WindowedApp` in Dist builds) that links against the `Engine` static library and hosts a dockable panel UI for inspecting and editing engine-side state — the active ECS scene, the asset directory tree, engine log output, and (nominally) a live scene viewport. It is not a runtime/player build: there is no separate "game" executable in the repository, and the Editor is currently the only executable target produced by the Premake build. `Editor::Application` derives from `Zephyr::Application` and supplies the panel set and ImGui chrome (main menu bar, dockspace, theme); the actual window, input, and rendering are owned by the Engine's module system, not by the Editor itself.

Note: the engine is mid-refactor on the `engine-overhaul` branch (module manager, RHI abstraction, `Renderer`/`Window` split all in flux), and several things described below — especially the ImGui wiring and the scene viewport — are visibly incomplete as a result. See "Design Notes" for specifics.

## Application Lifecycle

`Editor::Application` (`Editor/src/App.h`, `Editor/src/App.cpp`) is a `final` subclass of `Zephyr::Application` (`Engine/src/Zephyr/Core/Application.h`). The base class constructor does the heavy lifting before any Editor code runs: it registers a `Window` module (1920x1080, titled from `ApplicationSpecification::Name`), an `Input` module bound to the window's GLFW handle, and a `Renderer` module (hardcoded to `GraphicsAPI::VULKAN` via `RendererData`) with the engine's `ModuleManager`, then calls the virtual `OnInit()`.

`Editor::Application::OnInit()` (`App.cpp:12`):
- Calls `SetImGuiTheme()` to install a dark ImGui color palette and load the two editor fonts (`Resources/Fonts/CascadiaCode.ttf` as the base font, `Resources/Fonts/forkawesome-webfont.ttf` merged in for ForkAwesome icon glyphs, per `IconFontCppHeaders`/`IconsForkAwesome.h`).
- Creates the editor's own `Zephyr::ECS::Scene` (`m_Scene`), independent of any project/asset-loaded scene — there is no scene load/save path wired up yet (see Design Notes).
- Constructs the six panels (`InfoPanel`, `ProjectPanel`, `ConsolePanel`, `HierarchyPanel`, `AssetBrowserPanel`, `ScenePanel`) into `m_Panels`, a `std::vector<Zephyr::Scope<Panel>>`.

`Application` owns, directly: the panel vector and the editor scene. It does *not* own the window, device/renderer, or input — those live as modules inside `Zephyr::Application::m_ModuleManager` and are reached through `Zephyr::Application::Get()` / `GetModule<T>()` from panel code (e.g. `HierarchyPanel` reaches the active scene via `Zephyr::Application::Get().GetActiveScene()`, which `Editor::Application::GetActiveScene()` implements by returning `m_Scene`).

The base `Application::Run()` loop drives: `PreUpdate → Update → PostUpdate → PreRender → Render → PostRenderer` each frame, each step first running the `ModuleManager`'s per-module hook and then the corresponding `On*` virtual on `Application`. `Editor::Application` only overrides `OnUpdate` and `OnImGui`:
- `OnUpdate(deltaTime)` iterates `m_Panels` and calls `panel->OnUpdate()` unconditionally (even for closed/inactive panels — see Design Notes).
- `OnImGui(deltaTime)` draws the main menu bar and dockspace, then calls `panel->OnImGui()` for each panel where `panel->IsActive()` is true.
- `OnShutdown()` is empty.

## Panels

### `Panel` / `Panels.h` (base abstraction)

`Panel` (`Editor/src/Panels/Panel.h`) is the common base: a name (`Zephyr::String`), a `PanelCategory` (`WINDOW` or `INFO`, declared as `BIT(0)`/`BIT(1)` but never combined — each panel has exactly one category and comparisons are plain equality, not bitmask tests, so the flag-style declaration is unused as flags), an `m_Open` bool, and the pure-virtual `OnUpdate()`/`OnImGui()` pair every panel implements. `SwitchActive()` toggles visibility and is invoked from the main menu bar's per-panel checkbox items. `Panels.h` is a pure include-aggregator with no other content — it just pulls in the six concrete panel headers so `App.cpp` can `#include <Panels/Panels.h>` once.

### AssetBrowserPanel

Displays the active project's `Assets` directory (`Zephyr::Project::GetActive()->GetWorkingDirectory()`) as a two-pane browser: a recursive folder tree on the left (`FolderBrowser`) and a thumbnail-style grid of files/subfolders in the current directory on the right (`FileBrowser`), plus a filter box for each. It rebuilds its in-memory `Folder`/`File` tree (`UpdateDirectories`/`UpdateFolders`) both on directory change and by polling `std::filesystem::last_write_time` on the current directory and on the project root every `OnUpdate()` — this is a simple filesystem-watch substitute, not an actual OS file-watcher. Files are drag-and-drop sources (`ASSET_ITEM` payload, presumably intended for drop targets elsewhere in the editor — no consumer currently exists in this panel set) and double-clicking a file shells out via `Zephyr::FileDialogs::OpenFile` (i.e. opens it with the OS default handler rather than any in-editor viewer). The context menu supports creating a folder and deleting the selected file, both applied directly via `std::filesystem` calls (no undo, no asset-database bookkeeping). A rename flag (`RenameAssets`) is declared but never read — dead state.

### ConsolePanel

Mirrors engine log output into an ImGui text buffer. It registers itself with `Zephyr::Log::SetLogCallback` in its constructor, so every `Log::` call anywhere in the engine gets appended here with its `LogLevel` tracked per line (line-to-level mapping via parallel `ImVector`s) for level-colored rendering and text filtering. Provides Clear, Copy-to-clipboard, and a text filter. The "Save log" button is a stub: it opens a save-file dialog and appends a `.log` extension to the chosen path but never actually writes the buffer to disk — the write call is commented out (`//Blob buffer = Blob(...)`).

### HierarchyPanel

The most functionally complete panel; combines an entity list ("Hierarchy" window) with a component inspector ("Inspector" window, a second, separately-`Begin`'d ImGui window drawn from the same `OnImGui()` call). Pulls the current scene each frame via `Zephyr::Application::Get().GetActiveScene()`, then iterates `Scene::m_Registry.each(...)` directly — `Scene` explicitly `friend`s `Editor::HierarchyPanel` (see `Engine/src/Zephyr/ECS/Scene.h:82`) specifically to allow this raw EnTT registry access, rather than exposing it through a general accessor on `Scene`'s public API. Supports creating/deleting entities, renaming via the `TagComponent`, and adding/removing `TransformComponent`, `MeshComponent`, and `LightComponent` (add-component menu offers only `MeshComponent`/`LightComponent` — `TransformComponent` is presumably always present on creation). Component-specific UI: `TransformComponent` gets the classic colored X/Y/Z drag-float rows (`DrawVec3Control`); `MeshComponent` edits raw mesh/material integer IDs (no asset picker); `LightComponent` edits type (directional/point — the combo's "currently selected" tracking uses a static `const char*` compared by pointer identity against a local `types[]` array rather than by index or scene state, which will misbehave once more than one light is inspected in a session), color, and direction.

Gizmo support (`ImGuizmo`, translate/rotate/scale + world/local mode radio buttons) is present in the UI but `DrawGizmos()`'s actual `ImGuizmo::Manipulate` call, and the camera/view/projection plumbing it needs, are entirely commented out — the radio buttons currently have no visible effect on anything, since `m_GizmoType` defaults to `ImGuizmo::BOUNDS` and `DrawGizmos()` only acts when it isn't. `OnUpdate()` is also entirely commented-out camera-follow logic. Both are clearly not-yet-reconnected after the renderer refactor rather than intentionally removed.

### InfoPanel

An engine/performance stats window. Tracks rolling 100-sample histories of FPS and delta time (`Zephyr::Time::GetFPS()`/`GetDeltaTime()`) and plots them with `ImGui::PlotLines`. The "Renderer" collapsing header is almost entirely commented out (VRAM usage plot, device name/vendor, graphics API name) pending the `DeviceManager`/`RenderDevice` API that used to expose this pre-refactor; only a hardcoded `"Deferred renderer"` text line survives. `m_VramUsage` is declared but, with its only producer commented out, never populated.

### ProjectPanel

A stub. `OnUpdate()` is empty; `OnImGui()` only does `ImGui::Begin(m_Name.c_str(), &m_Open); ImGui::End();` — an empty window with no content. Given its name and the presence of `Zephyr::Project` elsewhere in the Editor (used by `AssetBrowserPanel`), this panel is presumably meant to surface/edit project settings but currently displays nothing.

### ScenePanel

Intended to host the 3D scene viewport as an ImGui image (render target from the renderer, `nvrhi::FramebufferHandle`), with pan/resize handling for the viewport texture. As currently checked in, `m_ViewPort` is never assigned — the line that would fetch it from the renderer is commented out in the constructor (`//m_ViewPort = Zephyr::Application::Get().GetRenderer().GetViewPort();`) — so `OnImGui()` always takes the early-return branch and renders only the text "No viewport available". All of the resize/image-draw logic below that check (`m_ViewPort->Resize(...)`, `ImGui::Image(m_ViewPort->GetImGuiAttachment(0), ...)`) is unreachable dead code under the current build. This panel is effectively non-functional pending the renderer/RHI work landing on this branch.

## Design Notes

- **The ImGui pass is not wired into the render loop.** `Zephyr::Application::Render()` (`Engine/src/Zephyr/Core/Application.cpp:108-113`) has `OnImGui(deltaTime)` commented out. Since `Editor::Application::OnImGui` is where all panel drawing happens, and nothing else in the Engine currently calls it, the entire panel UI described above is presently dead code at runtime — it will not appear on screen until this call is restored (or the panels are rehomed to whatever hook replaces it, e.g. `ImGuiRenderPass::buildUI`, which exists in `Engine/src/Zephyr/Renderer/RenderPasses/ImGuiRenderPass.h` but currently has no override anywhere that calls into the Editor's panel set). This is almost certainly a byproduct of the in-progress module/renderer refactor rather than an intentional removal.
- **`OnShutdown()` is likewise never invoked.** `Zephyr::Application::Close()` only calls `m_ModuleManager.Shutdown()`; it never calls the virtual `OnShutdown()`. `Editor::Application::OnShutdown()` is defined but empty regardless, so this has no present effect, but it means any future editor-side teardown logic added there won't run without also fixing `Close()`.
- **`OnUpdate` runs for inactive panels but `OnImGui` doesn't.** `Application::OnUpdate` iterates all panels unconditionally, while `OnImGui` filters on `IsActive()`. This is a reasonable pattern (panels can poll state while hidden) but is worth knowing when adding an expensive `OnUpdate()` — e.g. `AssetBrowserPanel::OnUpdate()` polls the filesystem every frame regardless of whether the panel is visible.
- **`PanelCategory` looks like a bitflag enum but isn't used as one.** Values are declared via `BIT(0)`/`BIT(1)`, but every panel has exactly one category and all call sites (`MainMenuBar`) compare with `==`, not `&`. A plain sequential enum would express the same thing without implying combinability that doesn't exist.
- **The editor scene is disconnected from `Zephyr::Project`.** `AssetBrowserPanel` tracks the active project's working directory and reacts to it changing, but `Editor::Application::OnInit()` creates a bare, empty `Scene` with no load-from-project / save-to-project path anywhere in this panel set — there's no "open scene" or "save scene" menu item yet. The `MainMenuBar`'s "About" menu has "Load dummy texture" / "Load model" items that are present but have empty bodies — placeholders, not working features.
- **`ScenePanel` and large parts of `HierarchyPanel`/`InfoPanel` are stubbed pending the renderer refactor.** All three have chunks of commented-out code that clearly used to talk to a previous `Renderer`/`RenderDevice`/camera API. Treat anything in this doc describing gizmo manipulation, the viewport image, or renderer/VRAM stats as "UI present, backing functionality removed mid-refactor," not as working behavior.
- **`ConsolePanel`'s save-log button doesn't save.** It resolves a destination path via `Zephyr::FileDialogs::SaveFile` but never writes to it.
- **`HierarchyPanel::DrawComponent` is called with the wrong label for `MeshComponent`.** `DrawComponents()` passes `"Transform"` as the header name for the `MeshComponent` block (`HierarchyPanel.cpp:329`) — almost certainly a copy-paste leftover from the `TransformComponent` block above it; the inspector will show two headers both labeled "Transform," the second one actually containing mesh/material fields.
- **`Scene` grants raw `entt::registry` access only to `Editor::HierarchyPanel`** via an explicit `friend` declaration, rather than exposing an iterator/view through its public API. This keeps EnTT out of `Scene`'s public interface everywhere else, at the cost of a hardcoded coupling from the Engine to one specific Editor class.

## Dependencies

**Engine modules/subsystems used directly by Editor code:**
- `Zephyr::Application` / `ApplicationSpecification` / `ModuleManager` (`Zephyr/Core/Application.h`) — base app class and lifecycle the Editor plugs into.
- `Zephyr::ECS::Scene` / `Entity` and its components (`TagComponent`, `TransformComponent`, `MeshComponent`, `LightComponent`) (`Zephyr/ECS/...`) — edited by `HierarchyPanel`.
- `Zephyr::Project` (`Zephyr/Project/Project.h`) — supplies the active project's working directory to `AssetBrowserPanel`.
- `Zephyr::FileSystem` / `Zephyr::FileDialogs` (`Zephyr/FileSystem/...`) — native open/save file dialogs used by `AssetBrowserPanel` and `ConsolePanel`.
- `Zephyr::Log` / `Zephyr::LogLevel` (`Zephyr/Core/Log.h`) — log sink consumed by `ConsolePanel` via `Log::SetLogCallback`.
- `Zephyr::Time` (`Zephyr/Time/Time.h`) — FPS/delta-time source for `InfoPanel`.
- `Zephyr::Input` (`Zephyr/Input/Input.h`) — included by `HierarchyPanel` (for gizmo snap-key checks in the currently-disabled `DrawGizmos` code).
- nvrhi (`Engine/Vendor/nvrhi`) — `ScenePanel` references `nvrhi::FramebufferHandle` directly for the (currently non-functional) viewport image.

**Editor-specific vendor libraries (`Editor/Vendor/`, added to the Editor project via `Build-Editor.lua`, not shared with Engine):**
- **ImGuizmo** (`Editor/Vendor/ImGuizmo`) — 3D transform gizmo widget; compiled directly into the Editor target (`Build-Editor.lua` lists `ImGuizmo.h`/`.cpp` explicitly in `files{}`). Used by `HierarchyPanel` for the translate/rotate/scale gizmo (presently non-functional — see Design Notes).
- **IconFontCppHeaders** (`Editor/Vendor/IconFontCppHeaders`, specifically its `FontIcons/IconsForkAwesome.h`) — ForkAwesome icon glyph constants (`ICON_FK_*`) used throughout the panels for toolbar/button icons, merged into the ImGui font atlas in `SetImGuiTheme()`.

The Editor also links the Engine's own ImGui build (`Engine/Vendor/ImGui`) and the rest of the Engine's third-party stack (GLFW, glm, spdlog, nvrhi, VulkanSDK — see `includedirs` in `Build-Editor.lua`) transitively through `links {"Engine"}`.
