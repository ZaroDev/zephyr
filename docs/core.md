# Core

The `Core` submodule is the engine's foundation layer: the application lifecycle, primitive
type aliases, platform detection, logging, assertions, allocation tracking, entity identity
(UUID), and the (currently unfinished) console-variable system. Every other Engine submodule,
and every Editor translation unit, transitively depends on `Core` — it is included implicitly
through the precompiled header (`pch.h`) and the umbrella header (`Zephyr.h`). Its job is to
give the rest of the codebase a consistent, platform-independent vocabulary (`u32`, `Ref<T>`,
`CORE_ASSERT`, etc.) before any higher-level system (ECS, Renderer, Modules) is allowed to
build on top of it.

**Branch note:** this is written against `engine-overhaul`, where `Application.h/.cpp` and
`Base.h` are mid-refactor and uncommitted. `Application` now owns a `ModuleManager` and drives
`Window`/`Input`/`Renderer` as registered modules rather than owning them directly; the old
`Zephyr/Renderer/GraphicsAPI.h/.cpp` files have been deleted in favor of a new `Zephyr/RHI`
tree. Treat anything below describing `Application` as a description of an in-progress design,
not a finished one.

## Key Files

| File | Purpose |
|---|---|
| `Core/Application.h` / `.cpp` | The `Application` base class: owns the main loop, the `ModuleManager`, and the update/render pipeline that subclasses (e.g. the Editor's app) hook into. |
| `Core/Base.h` | Fundamental macros and aliases: `DEBUGBREAK`, copy/move-defaulting macros, `Scope<T>`/`Ref<T>` smart-pointer aliases, `Path`/`String`/`StrView` typedefs, `BIT`, `BIND_EVENT_FN`. |
| `Core/Log.h` / `.cpp` | spdlog-backed logging: a core logger (`CORE_*` macros) and a client/app logger (bare macros), plus a callback hook so a UI (e.g. an editor console panel) can mirror log lines. |
| `Core/Assert.h` | `ASSERT`/`CORE_ASSERT` macros, compiled out entirely unless `ENABLE_ASSERTS` is defined. |
| `Core/CVarManager.h` / `.cpp` | An id-Tech/`AutoCVar`-style console variable system with an ImGui debug editor. Bool CVars work; int/float/string CVars are stubbed out (see Design Notes — this is a live bug). |
| `Core/UUID.h` / `.cpp` | A 64-bit random unique identifier type, used as the ECS entity identity key. |
| `Core/Allocations.h` | Global `operator new`/`operator delete` overrides for coarse allocation counting/tracking. |
| `Core/BasicTypes.h` | Fixed-width integer/float typedefs (`i8..i64`, `u8..u64`, `f32`, `f64`, `SizeT`) and their max-value constants. |
| `Core/PlatformDetection.h` | Compile-time platform detection macros. Only Windows x64 is actually supported; every other branch is `#error`. |
| `Core/EntryPoint.h` | Defines `main`/`WinMain` and the `Zephyr::EntryPoint()` function that a client application (the Editor) hooks into via `CreateApplication`. |
| `src/Zephyr.h` | Umbrella header re-exporting the engine's public surface (Core, Modules, Math types, Input, Time, Project). |
| `src/pch.h` / `pch.cpp` | The precompiled header. `pch.cpp` is just `#include <pch.h>` to generate the PCH object; `pch.h` pulls in STL containers plus `UUID`, `Assert`, `PlatformDetection`, `Base`, `BasicTypes`, `Math/MathTypes`, and `FileSystem/FileSystem.h`. |

## Key Types & APIs

### `Application` (`Application.h/.cpp`)
Abstract base class a client (the Editor) derives from. Construction:
- Asserts a singleton (`s_Instance`) doesn't already exist, sets `m_Running = true`, stores the `ApplicationSpecification`.
- Registers a `Window` module (1920x1080, titled from the spec), an `Input` module bound to the window's GLFW handle, and a `Renderer` module hard-coded to `GraphicsAPI::VULKAN`.
- Calls the virtual `OnInit()` hook.

`Run()` is the main loop: each iteration calls `Time::StartTimeUpdate()`, computes `deltaTime`,
re-sorts modules (`m_ModuleManager.SortModules()`), then walks a fixed pipeline of private
dispatch functions — `PreUpdate → Update → PostUpdate → PreRender → Render → PostRenderer` —
each of which first ticks the `ModuleManager` for that phase and then calls the matching
`On*` virtual so a subclass can hook in. Physics (`PrePhysics/Physics/PostPhysics`) and
`OnImGui` are wired up as virtuals/dispatch functions but are **not currently called** from
`Run()` — the physics phase is absent from the loop body and `OnImGui` is commented out inside
`Render()`. `GetActiveScene()` is a pure virtual the concrete application must implement so the
engine can query "what scene is currently active" without knowing about editor-specific state.

`GetModule<T>()` is a thin static forwarder to `ModuleManager::GetModule<T>()`, letting any code
reach a registered module (Window, Input, Renderer, ...) via `Application::Get().GetModule<T>()`.

`Close()` shuts down the module manager. `RequestClose()` just flips `m_Running = false` so the
loop exits after its current iteration; `Close()` must still be called afterward (see
`EntryPoint.h`) to actually tear modules down.

### Logging (`Log.h/.cpp`)
Two independent spdlog loggers — `s_CoreLogger` ("ZEPHYR", for engine code) and
`s_ClientLogger` ("APP", for game/editor code) — each writing to a color console sink and a
`Debug.log` file sink. `Log::Init()` must run before any log macro is used (done first thing in
`EntryPoint`). `Log::SetLogCallback` lets a UI layer subscribe to every log line; `AddLog`/
`AddCoreLog` re-read the *last* message from the sink and forward it through the callback,
which is what every `CORE_*`/client log macro does immediately after the spdlog call.

Custom `operator<<` overloads make `glm` vectors/matrices/quaternions printable via
`glm::to_string`, so they can be passed straight into `fmt`-style log calls.

### Assertions (`Assert.h`)
`CORE_ASSERT(expr, ...)` / `ASSERT(expr, ...)` expand, via macro overload dispatch on argument
count, to either a "condition text + file:line" message or a caller-supplied formatted message,
log it as an error through the core/client logger, and then call `DEBUGBREAK()`. Both macros
compile to nothing unless `ENABLE_ASSERTS` is defined. Per `Engine/Build-Engine.lua`,
`ENABLE_ASSERTS` is defined for `Debug` and `Release` configs but **not** `Dist` — so asserts,
including ones guarding invariants like double-initializing `Application`, silently vanish in
distribution builds.

### CVar system (`CVarManager.h/.cpp`)
A singleton (`CVarManager::Get()`, meyers-singleton via `CVarManagerImpl`) holding four
fixed-capacity (1000 entries each) arrays of typed CVar storage (bool/int/float/string).
`AutoCVar_Bool/_Int/_Float/_String` are the intended call-site API: declare one as a static/
member variable with a name, description, and default, then call `.Get()`/`.Set()`. The
manager also owns `DrawImGuiEditor()`, a debug UI that lists/searches/edits all registered
CVars (grouping them into ImGui submenus by a `category.name` dot-convention once there are
more than 10). See Design Notes for a serious bug here.

### `UUID` (`UUID.h/.cpp`)
A trivial wrapper around a random `u64` (via `std::mt19937_64` seeded from
`std::random_device`), implicitly convertible to `u64`, with a `std::hash` specialization so it
can key `std::unordered_map`/`unordered_set`. This is the entity-identity type used by
`ECS::IDComponent` — see `docs/ecs.md`.

### Allocation tracking (`Allocations.h`)
Overrides global `operator new`/`operator new[]`/`operator delete`/`operator delete[]` to bump
file-scope counters `s_AllocationCount`/`s_AllocationsSize`. This is header-only and included
from `Zephyr.h`, meaning it applies to **every** allocation in the process once linked in — see
Design Notes for a correctness bug in the size accounting.

### `BasicTypes.h`, `PlatformDetection.h`, `EntryPoint.h`, `Base.h`
Foundational, non-behavioral: type aliases, `#error`-based platform gating (Windows x64 only),
`main`/`WinMain` entry points that call `Log::Init()` then `CreateApplication` → `Run()` →
`Close()`, and the `DEFAULT_COPY`/`DISABLE_MOVE`/`Ref<T>`/`Scope<T>` macros and aliases used
throughout the rest of the engine.

## Design Notes

- **`ApplicationCommandLineArgs::operator[]` assert looks inverted.** It reads
  `CORE_ASSERT(Count < index)`, which is backwards from the intended bounds check
  (`index < Count`) — as written it asserts when `index` is *smaller* than `Count`, i.e. it
  fires on valid accesses and stays silent on out-of-bounds ones.
- **CVar system is half-implemented.** `CreateIntCVar`, `CreateFloatCVar`, and
  `CreateStringCVar` in `CVarManagerImpl` all unconditionally `return nullptr;` — only
  `CreateBoolCVar` is implemented. Since `AutoCVar_Int`, `AutoCVar_Float`, and `AutoCVar_String`
  immediately dereference the returned pointer (`cvar->Flags = flags;`), constructing any of
  those three types is a guaranteed null-pointer dereference today. Only `AutoCVar_Bool` is
  safe to use. `DrawImGuiEditor()`'s `EditParameter` also mixes CVar-array element types
  inconsistently (e.g. it reads `CVarType::INT` through `GetCVarArray<int32_t>()` in one branch
  but `GetCVarArray<u32>()` in the checkbox branch, and reads `CVarType::FLOAT`/`STRING` through
  `double`/`std::string` arrays that don't match the `f32`/`String` arrays actually declared on
  `CVarManagerImpl` — these are template instantiations of arrays that don't exist as members,
  so this code cannot currently compile if that path is exercised). No call site in the engine
  or editor currently constructs a `CVarManager`/`AutoCVar`, so this subsystem is effectively
  dead/unintegrated code right now.
- **`Allocations.h`'s size tracking is wrong.** `operator delete` does
  `s_AllocationsSize -= sizeof(pointer)`, which is `sizeof(void*)` (8 bytes on x64) regardless
  of the actual block size — it does not undo the `+= count` done in `operator new`. Over time
  `s_AllocationsSize` will drift arbitrarily instead of tracking live bytes. `s_AllocationCount`
  (block count) is correct. Neither counter is exposed via any accessor yet; they are dead
  writes-only state as of this reading.
- **`Time::GetDeltaTime()` is in milliseconds, not seconds** (see `docs/time.md`), and
  `Application::Run()` forwards that value directly as `deltaTime` to every `On*Update`/
  `On*Render` virtual. Any per-frame code written assuming seconds (the common engine
  convention) will be off by 1000x.
- **Physics pipeline is wired but unreachable.** `Application` declares
  `OnPrePhysicsUpdate`/`OnPhysicsUpdate`/`OnPostPhysicsUpdate` and private
  `PrePhysics`/`Physics`/`PostPhysics` dispatchers that forward to `ModuleManager`, but `Run()`
  never calls them. `OnImGui` is likewise declared and dispatchable but the call in `Render()`
  is commented out. Both read as intentionally-staged, not-yet-wired hooks for the
  ongoing engine-overhaul.
- **`EntryPoint.h`'s `WinMain` has a null-pointer bug.** It declares `char** argv = nullptr;`
  and then writes into `argv[i]` in a loop — this will crash immediately if the `DIST` +
  Windows `WinMain` path is ever actually exercised. This path also loops `i` as `u32` against
  `argc` (`int`), and never `LocalFree`s the `argvW` array `CommandLineToArgvW` returns.
- **`ENABLE_ASSERTS`/`DEBUG`/`RELEASE`/`DIST` are all build-time defines** set per-configuration
  in `Engine/Build-Engine.lua` (`Debug`: `DEBUG, ENABLE_ASSERTS`; `Release`: `RELEASE,
  ENABLE_ASSERTS`; `Dist`: `DIST` only). `DEBUGBREAK()` in `Base.h` is gated on `DEBUG`
  specifically (not `ENABLE_ASSERTS` or `_DEBUG`), so it is a no-op even in `Release` builds
  where asserts are otherwise active — an assert failure in `Release` logs an error but does not
  break into the debugger.
- **Platform support is intentionally narrow.** `PlatformDetection.h` `#error`s out on every
  target except 64-bit Windows (macOS, iOS, Android, Linux are all explicitly rejected). This is
  a hard compile-time wall, not a runtime check.
- **`Base.h`'s `ENUM_CLASS_FLAG_OPERATORS` macro is defined but not applied to any enum
  currently in scope** (`CVarFlags` in `CVarManager.h`, which looks like the natural
  candidate, does not use it — its flag checks are all done with manual casts, e.g.
  `(u32)parameter->Flags & (u32)CVarFlags::NO_EDIT`).
- **Copy/move policy is inconsistent by design, not oversight.** `Application` explicitly opts
  into `DEFAULT_MOVE_AND_COPY`, even though it also holds a raw self-pointer singleton
  (`s_Instance`) and non-copyable-in-spirit members like `ModuleManager` — copying an
  `Application` is very likely unsafe in practice despite being syntactically allowed.

## Dependencies

- **Depends on:** the C++ standard library (`<filesystem>`, `<memory>`, `<random>`, `<chrono>`
  indirectly via `Time`), `glm` (through `Math/MathTypes.h`, pulled in by `Log.h` for the
  stream operators), `spdlog` (`Log`), and vendored `imgui`/`imgui_stdlib` (`CVarManager`'s
  debug UI). `Application.cpp` also depends on `Modules/ModuleManager.h`, `Window/Window.h`,
  `Input/Input.h`, `Renderer/Renderer.h`, and `Time/Time.h` — i.e. `Core::Application` sits
  above those other submodules rather than below them.
- **Depended on by:** effectively everything. `Zephyr.h` and `pch.h` both pull in most of
  `Core`, so every Engine `.cpp` and every Editor `.cpp` gets it transitively. Direct,
  non-transitive consumers found in this pass include `ECS` (`UUID`, `Base.h`'s
  `DEFAULT_MOVE_AND_COPY`, `Assert.h`), `Editor/src/App.cpp`/`App.h` and the Editor panels
  (`Application::Get()`), `FileSystem` (`StringUtils` — technically `Utils`, but routed through
  `Core`-adjacent headers), and `Renderer`/`Window`/`Input` (basic types, `Ref`/`Scope`,
  asserts, logging).
