# RHI

`Engine/src/Zephyr/RHI/` is the engine's low-level graphics-device abstraction layer. It sits
directly on top of [nvrhi](https://github.com/NVIDIA-RTX/NVRHI) (vendored at
`Engine/Vendor/nvrhi`) and is the *only* thing above nvrhi that the rest of the engine talks to
when it needs a GPU device, a presentable surface, or command recording — it hides the
difference between D3D11, D3D12, and Vulkan behind a small, engine-owned interface instead of
exposing nvrhi (or raw D3D11/Vulkan) types everywhere. All three backends have real, working
implementations and are verified to render identically by switching a single `GraphicsAPI` value
(`Application.cpp`'s `RendererData::Api`). This is now the *only* device abstraction in the
engine — the older `Renderer/DeviceManager` stack (donut-derived, described in earlier revisions
of this doc) has been deleted; see `docs/renderer.md`.

## Key Files

| File | Purpose |
|---|---|
| `RHI/GraphicsAPI.h` / `.cpp` | `GraphicsAPI` enum (`D3D11`, `D3D12`, `VULKAN`), `GetGraphicsName()`, and `ToNVRHI(GraphicsAPI) -> nvrhi::GraphicsAPI`. |
| `RHI/Device.h` / `.cpp` | Abstract `Device` class + `Device::Create(GraphicsAPI)` factory (dispatches to `D3D11Device`/`D3D12Device`/`VulkanDevice`); `DefaultMessageCallback` bridges nvrhi log messages into the engine's `CORE_*` logging macros. |
| `RHI/Surface.h` | Abstract `Surface` interface: a presentable swapchain/window surface — `GetCurrentFrameIndex/GetWidth/GetHeight`, `Resize`, `AcquireNextImage`, `GetCurrentFramebuffer`, `Present`. Also defines `constexpr u32 c_FramesInFlight = 3`. |
| `RHI/CommandBuffer.h` / `.cpp` | `Zephyr::CommandList` — thin wrapper over `nvrhi::ICommandList` (open/close, `GetNvrhiCommandList()`). The only way engine code above nvrhi records and submits GPU work. |
| `RHI/D3D11/D3D11Device.h` / `.cpp`, `D3D11Surface.h` / `.cpp` | D3D11 backend: DXGI factory/adapter enumeration, `D3D11CreateDevice`, `nvrhi::d3d11::createDevice`. Swapchain is `DXGI_SWAP_EFFECT_FLIP_DISCARD` with an implicit single back buffer (`GetCurrentFrameIndex()` is always `0`) — D3D11's driver manages buffer reuse, so there's no explicit CPU/GPU sync here. |
| `RHI/D3D12/D3D12Device.h` / `.cpp`, `D3D12Surface.h` / `.cpp` | D3D12 backend: DXGI 1.4+ factory, `D3D12CreateDevice`, a single direct `ID3D12CommandQueue`, `nvrhi::d3d12::createDevice`. Swapchain has `c_FramesInFlight` buffers with **explicit per-frame fence sync** (one shared `ID3D12Fence` + a last-submitted value per back buffer index) since D3D12 gives no implicit guarantee a buffer is free to reuse. |
| `RHI/Vulkan/VulkanDevice.h` / `.cpp`, `VulkanSurface.h` / `.cpp` | Vulkan backend: `vk::Instance`/physical-device selection/`vk::Device`, exposing both a validation-wrapped `nvrhi::DeviceHandle` and a raw `nvrhi::vulkan::IDevice*` (needed for semaphore/native-texture interop the validation wrapper doesn't expose). Swapchain has `c_FramesInFlight` images with acquire/present semaphores plus an `EventQuery`-based CPU throttle so the app can't outrun the semaphore ring buffer. **This is the one file in the binary that defines `VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE`** (the global Vulkan dynamic dispatcher — required exactly once per binary). |

## Key Types & APIs

- **`Zephyr::GraphicsAPI`** (`RHI/GraphicsAPI.h`) — `enum class GraphicsAPI : u8 { D3D11, D3D12, VULKAN }`. `GetGraphicsName(GraphicsAPI)` returns a human-readable string. `ToNVRHI(GraphicsAPI)` converts to `nvrhi::GraphicsAPI`, used by `ShaderFactory`'s target-format selection (see `docs/renderer.md`).

- **`Zephyr::Device`** (`RHI/Device.h`) — abstract base class for a graphics device:
  - `static Ref<Device> Create(GraphicsAPI api)` — factory; all three `GraphicsAPI` values construct a real, working backend.
  - `virtual GraphicsAPI GetGraphicsAPI() const = 0`
  - `virtual Ref<Surface> CreateSurface(const Ref<Window>& window) = 0`
  - `virtual nvrhi::DeviceHandle GetNvrhiDevice() const = 0` — everything above nvrhi (`ShaderFactory`, PSO/resource creation) goes through this handle rather than a backend-specific type.
  - This is the type `Zephyr::Renderer` (see `docs/renderer.md`) owns and creates at startup.

- **`Zephyr::DefaultMessageCallback`** (`RHI/Device.h`/`.cpp`) — singleton (`GetInstance()`) implementing `nvrhi::IMessageCallback`; routes nvrhi's `Info/Warning/Error/Fatal` messages to `CORE_INFO/CORE_WARN/CORE_ERROR/CORE_CRITICAL`.

- **`Zephyr::Surface`** (`RHI/Surface.h`) — abstract interface representing a window's swapchain from the `Device`'s point of view. `AcquireNextImage()` must be called once per frame before `GetCurrentFramebuffer()`; `Present()` presents the current image. Each backend (`D3D11Surface`, `D3D12Surface`, `VulkanSurface`) has its own concrete implementation — swapchain mechanics differ enough per API (`IDXGISwapChain` vs. `VkSwapchainKHR`) that there's no useful shared base implementation beyond the interface.

- **`Zephyr::CommandList`** (`RHI/CommandBuffer.h`/`.cpp`) — constructed from an `nvrhi::DeviceHandle`; `Open()`/`Close()` wrap `nvrhi::ICommandList::open/close`. `Zephyr::Renderer::Render()` records into this each frame and submits via `Device::GetNvrhiDevice()->executeCommandList(...)`.

## Design Notes

- **RHI is the only device abstraction.** The older `Renderer/DeviceManager` stack (`DeviceManager`, `D3D11DeviceManager`, `VulkanDeviceManager`, `IRenderPass`, `ImGuiRenderPass`) — a donut-derived, more complete-looking parallel stack that was never wired into the running application — has been deleted. All device/swapchain code the engine actually runs lives under `RHI/`.

- **`.bin` shader cache is not keyed by `GraphicsAPI`.** `ShaderFactory` (see `docs/renderer.md`) caches compiled bytecode to `<shader>.bin` next to the `.slang` source, keyed only by filename. Switching the active `GraphicsAPI` without deleting the stale `.bin` files will silently feed the wrong bytecode format (DXBC/DXIL/SPIR-V) to `nvrhi::createShader`. This is a known, currently-unfixed limitation — see `tasks/todo.md` Task 10/11 notes.

- **ImGui rendering is not yet re-implemented against RHI.** Deleting `ImGuiRenderPass` (which was written against `DeviceManager`/`IRenderPass`) left no ImGui-via-nvrhi renderer in the tree. `Application::OnImGui` is currently disabled (commented out in `Application::Render()`). Re-adding it against `RHI::Device`/`RHI::CommandList`/`RHI::Surface` is tracked as Phase 6 in `tasks/plan.md`.

- **Ownership.** `Device` instances are reference-counted via `Ref<T>` and owned by whichever module creates them — currently `Zephyr::Renderer` (`Engine/src/Zephyr/Renderer/Renderer.h`), which holds `Ref<Device> m_DeviceHandle` for its own lifetime, plus `Ref<Surface> m_Surface` and `Scope<CommandList> m_CommandList`.

## Dependencies

**RHI depends on:**
- **nvrhi** (`Engine/Vendor/nvrhi`) — the actual GPU abstraction; `RHI/` is a thin engine-facing wrapper around it (`nvrhi::IDevice`, `nvrhi::d3d11`/`nvrhi::d3d12`/`nvrhi::vulkan` namespaces, `nvrhi::IMessageCallback`, etc.). Includes nvrhi's vendored `thirdparty/DirectX-Headers` (needed transitively by `nvrhi/d3d12.h`) and `Vulkan-Headers`.
- **Vulkan SDK** (`vulkan.hpp`, `nvrhi/vulkan.h`) — required by `VulkanDevice`/`VulkanSurface`; `VULKAN_SDK` env var is wired into include/lib paths in `Dependencies.lua`.
- **D3D11/D3D12/DXGI SDK headers** (Windows SDK) — required by the D3D11 and D3D12 backends. Linked via each backend's static-lib project in `Engine/Vendor/nvrhi/premake5.lua` (`d3d11.lib`/`dxgi.lib`/`d3dcompiler.lib`/`dxguid.lib` for `NVRHI-D3D11`; `d3d12.lib`/`dxgi.lib`/`dxguid.lib` for `NVRHI-D3D12`), which propagate to `Engine`/`Editor` through project references.
- `Zephyr::Window` (`Engine/src/Zephyr/Window/Window.h`) — `Device::CreateSurface` takes a `Ref<Window>`.
- `Zephyr::Core` basics — `Base.h` (`Ref`/`CreateRef`), `BasicTypes.h` (`u8`, `StrView`, etc.), the `CORE_*` logging macros used by `DefaultMessageCallback`.

**What depends on RHI:**
- `Zephyr::Renderer` (`Engine/src/Zephyr/Renderer/Renderer.h`/`.cpp`) — creates its `Device`/`Surface`/`CommandList` via `RHI::` in `Initialize()`, and drives the per-frame acquire/record/submit/present loop in `Render()`.
- Transitively, `Zephyr::Application` (`Engine/src/Zephyr/Core/Application.cpp`) registers `Renderer` as an `IModule`, and the `Editor` executable links `Engine` and runs through `Application`.
