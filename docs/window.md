# Window

The `Window` submodule owns the engine's OS window and the GLFW library lifecycle. It is a very small `IModule` wrapper around GLFW that creates a single top-level window, polls its OS events once per frame, and hands out the raw `GLFWwindow*` handle that other systems (rendering surface creation, native file dialogs, raw input polling) need. It exists to centralize "there is exactly one application window" instead of having every subsystem talk to GLFW independently.

## Key Files

| File | Purpose |
|---|---|
| `Window.h` | Declares `WindowParams` and the `Window` class (an `IModule`). |
| `Window.cpp` | Implements GLFW initialization/window creation/teardown and per-frame event polling. |

## Key Types & APIs

- **`WindowParams`** — a plain data struct: `Title` (default `"Window"`), `Width`/`Height` (default `1920x1080`), `Vsync` and `Fullscreen` booleans (both default `false`). Passed into `Window`'s constructor; stored as-is (`m_Params`) but note only `Title`, `Width`, and `Height` are actually consumed in `Window::Initialize` — `Vsync` and `Fullscreen` are logged but not applied to GLFW window hints (see Design Notes).

- **`Window`** (`final`, implements `IModule`):
  - `Initialize()` — calls `glfwInit()`, sets `GLFW_CLIENT_API` to `GLFW_NO_API` (i.e. GLFW is not asked to create any GL/Vulkan context — window creation is purely for a native surface handle, consistent with the engine's RHI/NVRHI-based renderer owning the graphics API), sets `GLFW_MAXIMIZED` true, creates the window via `glfwCreateWindow`, stores the window's own `WindowParams` as its GLFW user pointer, and installs a close callback that calls `Application::Get().RequestClose()`. Returns `false` (logged) if `glfwInit()` fails; asserts if window creation fails.
  - `Shutdown()` — `glfwDestroyWindow` then `glfwTerminate()`. Note `glfwTerminate()` is global GLFW library teardown, not window-specific — see Design Notes.
  - `GetGLFWHandle()` — returns the raw `GLFWwindow*`; this is the main way other systems reach into the window (e.g. RHI surface creation, `Input`'s constructor, the Windows file-dialog code for `HWND` lookup).
  - `IsOpen()` — `!glfwWindowShouldClose(...)`.
  - `PostRenderer(float deltaTime)` — overrides `IModule::PostRenderer`; calls `glfwPollEvents()`. This is the only per-frame work `Window` does, and it happens in the "post-renderer" phase of the module tick (see `GetUpdateFlags()` below).
  - `GetName()` → `"Window"`, `GetPriority()` → `0`, `GetUpdateFlags()` → `UpdateFlags::RendererUpdate` (so `ModuleManager::PreRender/Render/PostRenderer` will call into it, but `ModuleManager::PreUpdate/Update/PostUpdate` and the physics equivalents will not).

## Design Notes

- **New/untracked submodule.** `Window/` is untracked in git on this branch (`engine-overhaul`) — it's a brand-new addition, part of replacing a previous single `DeviceManager`-owned window with a dedicated `Window` module (see `Renderer`/`RHI` docs and the `Input`/`FileSystem` design notes for the corresponding call-site migrations).
- **Event polling happens in the render phase, not the update phase.** Because `GetUpdateFlags()` is `RendererUpdate` and `glfwPollEvents()` runs in `PostRenderer`, OS/input event pumping is tied to the render tick ordering, not a dedicated "platform" or "input" phase. Since `Input` polls GLFW state directly (rather than consuming buffered events) this mostly still works, but it does mean any code relying on `Update`-phase modules is querying window/input state that was last refreshed at the end of the *previous* frame's render phase.
- **`Vsync`/`Fullscreen` are accepted but unused.** `WindowParams::Vsync` and `Fullscreen` are logged in `Initialize()` (`CORE_INFO`) but never passed to any `glfwWindowHint` or `glfwSetWindowMonitor` call — the window is always windowed and always maximized (`GLFW_MAXIMIZED, GLFW_TRUE` is hardcoded). This looks like an incomplete feature: the knobs exist in the params struct and are surfaced in the log output, but nothing downstream honors them yet.
- **`glfwTerminate()` in a per-module `Shutdown()`.** `glfwTerminate()` tears down the entire GLFW library, not just this window. Since the engine only ever creates one `Window` module instance, this is currently safe, but it means `Window::Shutdown()` is implicitly assuming it's the last/only GLFW consumer being torn down — if a second `Window` (or any other GLFW-touching module) were ever added, shutdown ordering would matter.
- **GLFW user pointer stores a stack/member address by reference semantics.** `glfwSetWindowUserPointer(m_WindowHandle, &m_Params)` stores the address of the `Window`'s own `m_Params` member. Nothing currently reads this user pointer back (no `glfwGetWindowUserPointer` calls in the tree), so it's inert today, but it is a live pointer into the `Window` instance — if the `Window` were moved (it declares `DEFAULT_MOVE_AND_COPY`) the stored pointer would become stale.
- **Close handling is indirect.** The GLFW close callback doesn't set any window-local "should close" flag directly — it calls `Application::Get().RequestClose()`, which sets `Application::m_Running = false`, which stops `Application::Run()`'s loop. `Window::IsOpen()` (checking `glfwWindowShouldClose`) exists but isn't obviously used to drive the main loop; the loop's continuation is actually governed by `Application::m_Running`.
- **No resize/minimize/focus callbacks.** Only the close callback is registered. Consumers needing to react to resizes (e.g. swapchain recreation in the renderer/RHI layer) would need to either poll the window size each frame or the renderer would need to register its own GLFW callbacks — not present in this file.

## Dependencies

- **Depends on:** `Zephyr/Modules/IModule.h`, GLFW (`glfw3.h`), `Zephyr/Core/Application.h` (for `Application::Get().RequestClose()`).
- **Depended on by:** `Application` (constructs and registers the `Window` module first, before `Input` and `Renderer`, and passes it into `RendererData::Window`); `Input` (constructed with `window->GetGLFWHandle()`); `Zephyr/RHI/Device.h` (`CreateSurface(const Ref<Window>& window)` — the RHI/Renderer layer, documented separately, takes a `Ref<Window>` to create its rendering surface); the Windows file-dialog code in `FileSystem` (`Application::Get().GetModule<Window>()->GetGLFWHandle()` for `glfwGetWin32Window`).
