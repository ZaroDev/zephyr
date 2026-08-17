# Input

The `Input` submodule exposes keyboard, mouse-button, and cursor state to the rest of the engine. It is a thin, direct wrapper over GLFW's polling API — it does not buffer or cache state itself, and it does not process input events/callbacks. It exists so gameplay/editor code queries input through the engine's own type-safe key/button enums (`KeyCode`, `MouseButton`) rather than talking to GLFW directly, and so input is resolved against the specific window the engine created.

## Key Files

| File | Purpose |
|---|---|
| `Input.h` | Declares the `Input` class (an `IModule`) and its query API. |
| `Input.cpp` | Implements the query API as direct calls into GLFW. |
| `KeyCodes.h` | Defines `KeyCode`/`Key`, `MouseButton`/`Button`, `KeyState`, and `CursorMode` enums, mirrored from `glfw3.h`'s key constants. |

## Key Types & APIs

- **`Input`** (`Input.h`) — a `final` class implementing `IModule`. It is constructed with a `GLFWwindow*` and stores it for the lifetime of the module:
  - `bool IsKeyDown(KeyCode keycode)` — `glfwGetKey(...)`, true for `GLFW_PRESS` or `GLFW_REPEAT`.
  - `bool IsMouseButtonDown(MouseButton button)` — `glfwGetMouseButton(...)`, true for `GLFW_PRESS`.
  - `V2 GetMousePosition()` — `glfwGetCursorPos(...)`, returned as a `Zephyr::V2` (float 2-vector).
  - `void SetCursorMode(CursorMode mode)` — `glfwSetInputMode(..., GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode)`.
  - As an `IModule`: `GetName()` returns `"Input"`, `GetUpdateFlags()` returns `UpdateFlags::None` (it is never ticked by the `ModuleManager` — see Design Notes), and `IsCoreModule()` returns `true`.

- **Usage pattern.** `Input` is registered with the `ModuleManager` in `Application::Application(...)` (`Application.cpp`) immediately after the `Window` module, passing `window->GetGLFWHandle()`. Callers fetch it via `Application::Get().GetModule<Input>()` and call the query methods directly (e.g. `Application::Get().GetModule<Input>()->IsKeyDown(Key::LeftControl)`).

- **`KeyCodes.h`** — `KeyCode` (aliased `Key`) is a scoped `enum class : uint16_t` whose values are copied 1:1 from GLFW's key codes (documented in the header as "From glfw3.h"), so they can be passed straight into `glfwGetKey`. `MouseButton` (aliased `Button`) similarly mirrors `GLFW_MOUSE_BUTTON_*`. `KeyState` (`None`/`Pressed`/`Held`/`Released`) and `CursorMode` (`Normal`/`Hidden`/`Locked`) are also declared here; `CursorMode`'s values are designed to be added directly onto `GLFW_CURSOR_NORMAL` in `SetCursorMode`.

## Design Notes

- **Polling only, no state caching or edge-detection.** Every call goes straight to GLFW at call time; there is no per-frame snapshot, so there's no way to distinguish "just pressed this frame" from "held" using this API alone — `KeyState::Pressed`/`Held`/`Released` are declared in `KeyCodes.h` but nothing in `Input` currently produces or consumes them. Callers wanting edge-triggered input have to track previous-frame state themselves.
- **Never ticked by the module system.** `Input::GetUpdateFlags()` returns `UpdateFlags::None`, so `ModuleManager` never calls `PreUpdate`/`Update`/`PostUpdate`/etc. on it (those are gated on `UpdateFlags::Update`, `PhysicsUpdate`, or `RendererUpdate`). This is consistent with the module being a stateless pass-through to GLFW — it has no per-frame work to do — but it does mean `Input` is really just "a `Ref<T>`-accessible service located via the module registry," not a participant in the update loop.
- **Mid-refactor: was a static/free-function API.** Uncommitted changes on this branch (`git diff` against `Input.h`/`Input.cpp`) show `Input` used to be a `namespace Zephyr::Input { bool IsKeyDown(...); ... }` of free functions that reached a window handle through `Application::Get().GetDeviceManager().GetWindow()`. It has just been converted into an `IModule`-based instance class that owns its own `GLFWwindow*`, as part of the broader move away from a single global `DeviceManager` toward the `Window`/`Renderer`/`Input` module split. A stale call site using the old free-function style (`Zephyr::Input::IsKeyDown(Zephyr::Key::LeftControl)`) still exists in `Editor/src/Panels/HierarchyPanel.cpp`, but it's inside a commented-out block, so it doesn't currently break the build — it's a leftover from before this refactor and will need updating (to `GetModule<Input>()->IsKeyDown(...)`) whenever that gizmo code is re-enabled.
- **No input event/callback system.** There are no GLFW key/mouse callbacks registered anywhere for gameplay input (only `Window::Initialize` registers a `glfwSetWindowCloseCallback`); anything needing "on key pressed" semantics must poll `IsKeyDown` itself each frame.
- **Ownership.** `Input` does not own the `GLFWwindow*` — `Window` does (created/destroyed in `Window::Initialize`/`Shutdown`). `Input` simply holds a non-owning pointer, so `Input` is implicitly dependent on `Window` outliving it; the `ModuleManager` registers `Window` before `Input` in `Application`'s constructor, but there's no explicit dependency/ordering enforcement beyond that call order and `SortModules()`'s priority sort (both modules currently have priority `0`, so their relative tick order — irrelevant here since `Input` isn't ticked — falls to `std::sort`'s tie-breaking).

## Dependencies

- **Depends on:** `Zephyr/Modules/IModule.h` (base interface), `Zephyr/Window` (indirectly, via the `GLFWwindow*` it's constructed with — no compile-time include, but a lifetime dependency), GLFW (`glfwGetKey`, `glfwGetMouseButton`, `glfwGetCursorPos`, `glfwSetInputMode`), `Zephyr/Math/MathTypes.h` (`V2`), `Zephyr/Core/Application.h`.
- **Depended on by:** `Editor` panels that need keyboard/mouse state (e.g. gizmo snapping in `HierarchyPanel.cpp`, currently in a disabled code path); registered and owned by `Application`. `Zephyr.h` (the engine's umbrella header) includes `Input.h`, so any code including the engine header has the types available.
