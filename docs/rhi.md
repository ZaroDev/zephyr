# RHI

`Engine/src/Zephyr/RHI/` is the engine's new, low-level graphics-device abstraction layer. It sits directly on top of [nvrhi](https://github.com/NVIDIA-RTX/NVRHI) (vendored at `Engine/Vendor/nvrhi`) and is meant to be the *only* thing above nvrhi that the rest of the engine talks to when it needs a GPU device, a presentable surface, or command recording — i.e. it exists to hide the difference between D3D11, D3D12 and Vulkan behind a small, engine-owned interface instead of exposing nvrhi (or worse, raw D3D11/Vulkan) types everywhere. As of this branch it is a skeleton: the types and factory functions are in place, but only Vulkan has a (mostly empty) concrete implementation, and several methods are unimplemented stubs. See "Design Notes" below for the current state and how this relates to the older, more complete `Renderer/DeviceManager` stack.

## Key Files

| File | Purpose |
|---|---|
| `RHI/GraphicsAPI.h` / `.cpp` | `GraphicsAPI` enum (`D3D11`, `D3D12`, `VULKAN`) and `GetGraphicsName()` helper. The graphics-backend selector used throughout the engine. |
| `RHI/Device.h` / `.cpp` | Abstract `Device` class + `Device::Create(GraphicsAPI)` factory; `DefaultMessageCallback` bridges nvrhi log messages into the engine's `CORE_*` logging macros. |
| `RHI/Surface.h` | Abstract `Surface` interface representing a presentable swapchain/window surface, plus the `c_FramesInFlight` constant. Header-only, no implementation anywhere yet. |
| `RHI/CommadBuffer.h` | **Note the filename typo — "Commad" not "Command", kept as-is in the source tree.** Currently an empty placeholder (`namespace Zephyr {}`) — no command-buffer/command-list abstraction has been started yet; code that needs to record commands currently goes straight to `nvrhi::ICommandList`. |
| `RHI/Vulkan/VulkanDevice.h` / `.cpp` | The only concrete `Device` implementation. Declares the Vulkan instance/physical-device/nvrhi-device members but every method body is empty. |
| `RHI/D3D11/`, `RHI/D3D12/` | Empty directories. Present on disk (so the folder layout mirrors `GraphicsAPI`/`Renderer/Platform`) but contain no files — D3D11 and D3D12 `Device` backends do not exist yet. |

## Key Types & APIs

- **`Zephyr::GraphicsAPI`** (`RHI/GraphicsAPI.h`) — `enum class GraphicsAPI : u8 { D3D11, D3D12, VULKAN }`. Decorated with `ENUM_CLASS_FLAG_OPERATORS`, even though it is used as a plain selector rather than a bitmask — likely boilerplate copied from another enum rather than an intentional flags design. `GetGraphicsName(GraphicsAPI)` returns a human-readable string, used for logging/UI.

- **`Zephyr::Device`** (`RHI/Device.h`) — abstract base class for a graphics device:
  - `static Ref<Device> Create(GraphicsAPI api)` — factory. Only the `VULKAN` case returns anything (`CreateRef<VulkanDevice>()`); the `D3D11` and `D3D12` cases just `break` out of the switch with no `return`, so the function falls off the end without a value for those APIs (undefined behavior / MSVC `C4715` "not all control paths return a value"). Calling `Device::Create(GraphicsAPI::D3D11)` today is a bug waiting to happen, not a supported path.
  - `virtual GraphicsAPI GetGraphicsAPI() const = 0`
  - `virtual Ref<Surface> CreateSurface(const Ref<Window>& window) = 0`
  - This is the type `Zephyr::Renderer` (see `docs/renderer.md`) actually owns and creates at startup.

- **`Zephyr::DefaultMessageCallback`** (`RHI/Device.h`/`.cpp`) — singleton (`GetInstance()`) implementing `nvrhi::IMessageCallback`; routes nvrhi's `Info/Warning/Error/Fatal` messages to `CORE_INFO/CORE_WARN/CORE_ERROR/CORE_CRITICAL` respectively. This is the standard hook point for making nvrhi's internal validation/debug output show up in the engine's own logs.

- **`Zephyr::Surface`** (`RHI/Surface.h`) — abstract interface: `GetCurrentFrameIndex()`, `GetWidth()`, `GetHeight()`, `Resize(width, height)`. Intended to represent a window's swapchain from the `Device`'s point of view. No class implements it yet — `VulkanDevice::CreateSurface()` returns a default-constructed (null) `Ref<Surface>()`.

- **`Zephyr::VulkanDevice`** (`RHI/Vulkan/VulkanDevice.h`/`.cpp`) — `final class VulkanDevice : public Device`. Holds `vk::Instance`, `vk::DebugReportCallbackEXT`, `vk::PhysicalDevice`, and nvrhi's `nvrhi::vulkan::DeviceHandle` / `nvrhi::DeviceHandle` (validation-layer wrapper) members, mirroring what a real Vulkan device backend needs. In practice:
  - Constructor and destructor bodies are empty.
  - `CreateVulkanInstance()` (private) is declared and called nowhere, body empty.
  - `CreateSurface()` just returns `Ref<Surface>()` (null).
  - `GetGraphicsAPI()` is the only fully working method, returning `GraphicsAPI::VULKAN`.
  
  In short: `VulkanDevice` is a scaffold that compiles and can be constructed via `Device::Create(GraphicsAPI::VULKAN)`, but does not actually create a Vulkan instance, pick a physical device, or create an nvrhi device yet.

## Design Notes

- **This replaces the old `Renderer/GraphicsAPI.{h,cpp}`.** Git history shows `Engine/src/Zephyr/Renderer/GraphicsAPI.h`/`.cpp` were deleted and re-created under `Engine/src/Zephyr/RHI/`. The old version additionally had a `nvrhi::GraphicsAPI ToNVRHI(GraphicsAPI api)` conversion function; the new `RHI/GraphicsAPI.h` **dropped that function** (a repo-wide grep for `ToNVRHI` finds zero references anywhere in the current tree). Nothing in `RHI/` currently needs to convert `Zephyr::GraphicsAPI` to `nvrhi::GraphicsAPI` because `Device`/`VulkanDevice` don't yet expose an `nvrhi::IDevice*` at all — but the moment RHI grows a method like `ShaderFactory` needs (which takes `nvrhi::GraphicsAPI` to pick DXBC/DXIL/SPIR-V), that conversion will need to be re-added here. Treat its absence as a known gap, not an intentional simplification.

- **RHI vs. `Renderer/DeviceManager` — two parallel device abstractions currently coexist.** `Engine/src/Zephyr/Renderer/DeviceManager.h`/`.cpp` (see `docs/renderer.md`) is a much older, far more complete device/swapchain/render-loop abstraction ported from NVIDIA's donut/nvrhi example framework, with fully working `D3D11DeviceManager` and `VulkanDeviceManager` subclasses (real instance/device/swapchain creation, adapter enumeration, DPI handling, a render-pass list, etc.). `RHI::Device`/`VulkanDevice` is a from-scratch, much smaller replacement for that same responsibility, but only Vulkan has a stub, and even that stub does nothing. Grepping the whole repo confirms `DeviceManager::Create`, `D3D11DeviceManager`, and `VulkanDeviceManager` are **never instantiated** anywhere in `Editor/` or in `Application.cpp` — the only device the running application actually creates is `RHI::Device::Create(...)`, called from `Zephyr::Renderer::Initialize()` (`Engine/src/Zephyr/Renderer/Renderer.cpp`). So: `RHI/` is the live, intended-to-be-final path (currently non-functional beyond object construction); `Renderer/DeviceManager` + its platform subclasses are orphaned legacy code that still compiles and is more feature-complete, but nothing wires it up anymore. A future engineer either needs to finish porting `VulkanDeviceManager`'s real Vulkan setup logic into `RHI::VulkanDevice`/a future `RHI::Surface` implementation, or decide to resurrect `DeviceManager` instead — right now both exist and only one is actually reachable at runtime.

- **`Surface` is unimplemented.** Nothing in the tree provides a concrete `Surface`. Until one exists, `Device::CreateSurface()` can't produce anything a swapchain-consumer (e.g. a future `Renderer::Render()`) could use, and `Renderer::Initialize()` doesn't even call `CreateSurface()` today — it only creates the `Device`.

- **`CommadBuffer.h` is a placeholder.** It has no declarations at all. If/when it's filled in, expect it to wrap `nvrhi::ICommandList` the way `Device` wraps `nvrhi::IDevice`. Keep the typo in mind when searching the codebase or citing the path — the file is genuinely named `CommadBuffer.h`, not `CommandBuffer.h`.

- **D3D11/D3D12 have no RHI backend.** The `RHI/D3D11/` and `RHI/D3D12/` folders exist (so the directory shape already matches `GraphicsAPI`), but are empty. `Device::Create` will misbehave (see above) if asked for either. D3D11 support currently only exists in the old `Renderer/Platform/D3D11/D3D11DeviceManager`, which is not connected to `RHI::Device`.

- **Ownership.** `Device` instances are reference-counted via `Ref<T>` (the engine's shared-ownership smart pointer alias) and owned by whichever module creates them — currently `Zephyr::Renderer` (`Engine/src/Zephyr/Renderer/Renderer.h`), which holds `Ref<Device> m_DeviceHandle` for its own lifetime.

## Dependencies

**RHI depends on:**
- **nvrhi** (`Engine/Vendor/nvrhi`) — the actual GPU abstraction; `RHI/` is a thin engine-facing wrapper around it (`nvrhi::IDevice`, `nvrhi::vulkan::DeviceHandle`, `nvrhi::IMessageCallback`, etc.).
- **Vulkan SDK** (`vulkan.hpp`, `nvrhi/vulkan.h`) — required by `VulkanDevice`; `VULKAN_SDK` env var is wired into include/lib paths in `Dependencies.lua`.
- `Zephyr::Window` (`Engine/src/Zephyr/Window/Window.h`) — `Device::CreateSurface` takes a `Ref<Window>`.
- `Zephyr::Core` basics — `Base.h` (`Ref`/`CreateRef`, `ENUM_CLASS_FLAG_OPERATORS`), `BasicTypes.h` (`u8`, `StrView`, etc.), the `CORE_*` logging macros used by `DefaultMessageCallback`.

**What depends on RHI:**
- `Zephyr::Renderer` (`Engine/src/Zephyr/Renderer/Renderer.h`/`.cpp`) — the only current consumer; it creates its `Device` via `RHI::Device::Create()` in `Initialize()`.
- Transitively, `Zephyr::Application` (`Engine/src/Zephyr/Core/Application.cpp`) registers `Renderer` as an `IModule`, and the `Editor` executable links `Engine` and runs through `Application`, so `Editor` depends on RHI existing and compiling, but does not reference RHI types directly today.
