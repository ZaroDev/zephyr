# Renderer

`Engine/src/Zephyr/Renderer/` is where Zephyr's rendering code lives: camera math, shader
compilation/loading, and the `Renderer` `IModule` that owns the frame loop. It sits on top of
`Zephyr::RHI` (see `docs/rhi.md`), which is the engine's only device/swapchain abstraction — the
older, donut-derived `DeviceManager`/`D3D11DeviceManager`/`VulkanDeviceManager`/`IRenderPass`/
`ImGuiRenderPass` stack described in earlier revisions of this doc has been deleted.

## Key Files

| File | Purpose |
|---|---|
| `Renderer/Camera.h` / `.cpp` | `Camera` class: view/projection matrices, FOV, near/far planes, mouse-driven orbit/fly controls (currently disabled — see Design Notes). |
| `Renderer/Renderer.h` / `.cpp` | `Renderer : IModule` — the class registered with `Application`'s `ModuleManager`. Owns a `Ref<RHI::Device>`, `Ref<RHI::Surface>`, `Scope<RHI::CommandList>`, and `Scope<ShaderFactory>`. `Initialize()` creates the device/surface/command list, compiles the triangle shaders, and builds the pipeline on first frame; `Render()` acquires the next swapchain image, records a clear + draw into the command list, submits, and presents. |
| `Renderer/ShaderFactory.h` / `.cpp` | Compiles `.slang` shader source to platform bytecode (DXBC/DXIL/SPIR-V) via the Slang compiler API, caches compiled bytecode to `.bin` files, and wraps `nvrhi::IDevice::createShader`/`createShaderLibrary`. Constructed from an `nvrhi::DeviceHandle` (via `RHI::Device::GetNvrhiDevice()`), not tied to any specific backend. |

## Key Types & APIs

- **`Zephyr::Camera`** (`Camera.h`/`.cpp`) — holds `Mat4` projection/view/inverse matrices, FOV/near/far, position, forward direction, and an orbit target. `OnUpdate(ts)` is meant to poll `Zephyr::Input` for fly/orbit controls and recompute the view matrix; `OnResize(w, h)` recomputes projection. Despite the class doc-comment ("Definition of a 3D orthographic camera") and the `CameraParameters` (`Top/Bottom/Left/Right`) it computes in `OnResize`, `RecalculateProjection()` actually builds a **perspective** projection via `glm::perspectiveFov` and never reads `m_Parameters` — the orthographic bounds are computed but unused. Treat the "orthographic" doc-comment as stale.

- **`Zephyr::Renderer`** (`Renderer.h`/`.cpp`) — `final class Renderer : public IModule`. Constructed with `RendererData { GraphicsAPI Api; Ref<Window> Window; }`. `Initialize()`:
  1. `Device::Create(m_Data.Api)` — constructs the backend device (`D3D11Device`/`D3D12Device`/`VulkanDevice`).
  2. `m_DeviceHandle->CreateSurface(m_Data.Window)` — constructs the backend swapchain (`D3D11Surface`/`D3D12Surface`/`VulkanSurface`).
  3. Creates a `CommandList` and a `ShaderFactory` (base path `"Shaders"` — relative to the process working directory, which is `Assets/` for the Editor).
  4. Compiles `Triangle.vs.slang`/`Triangle.ps.slang` via `ShaderFactory`.

  `Render(deltaTime)`: polls the window's framebuffer size and calls `Surface::Resize()` if it changed; `AcquireNextImage()`; lazily builds the graphics pipeline on first use (cull-none, depth disabled, triangle-list); records a clear + `draw(3 vertices)` into the `CommandList`; submits via `Device::GetNvrhiDevice()->executeCommandList(...)`; `Surface::Present()`. No `GraphicsAPI`-specific branches exist anywhere in this file — verified across all three backends by switching only `Application.cpp`'s `RendererData::Api`.

  `GetPriority()` returns `1` and `IsCoreModule()` returns `true`, so `ModuleManager` treats a failed `Renderer::Initialize()` as fatal (`CORE_ASSERT`).

- **`Zephyr::ShaderFactory`** (`ShaderFactory.h`/`.cpp`) — constructed with an `nvrhi::DeviceHandle`, a `Ref<IFileSystem>`, and a base shader path. `CompileShader(fileName, entryName)` drives the Slang compiler (`slang::createGlobalSession`, `ISession`, `IModule`, `IEntryPoint`) to produce DXBC (`sm_5_0` — the device is created at `D3D_FEATURE_LEVEL_11_1`, which only supports Shader Model 5.0) for D3D11, DXIL (`sm_6_7`) for D3D12, or SPIR-V (`spirv_1_5`) for Vulkan — selected via `m_Device->getGraphicsAPI()` — and writes the result to a `.bin` file next to the source via `IFileSystem`. `CreateShader`/`CreateShaderLibrary` load (compiling on cache miss) and wrap bytecode into `nvrhi::ShaderHandle`/`ShaderLibraryHandle`. `CreateStaticShader`/`CreateStaticPlatformShader` variants accept pre-baked bytecode blobs instead of going through the filesystem. `CreateAutoShader`/`CreateAutoShaderLibrary` try the static path first and fall back to file-based compilation. `FindShaderFromHash(...)` is a declared-but-stub API — it unconditionally returns `{nullptr, 0}`. Only `.slang` files pass `IsShaderExtension`.

## Design Notes

- **One device stack.** `Renderer` talks exclusively to `RHI::Device`/`RHI::Surface`/`RHI::CommandList`. The previous state of this doc described two coexisting device abstractions (`DeviceManager` vs. the new `RHI`-based `Renderer`); that ambiguity is resolved — `DeviceManager` and everything built on it (`D3D11DeviceManager`, `VulkanDeviceManager`, `IRenderPass`, `ImGuiRenderPass`) has been deleted from the tree.

- **`.bin` shader cache is filename-keyed only, not per-`GraphicsAPI`.** Switching the active backend without deleting the stale `.bin` files feeds the wrong bytecode format to `nvrhi::createShader` (DXBC/DXIL/SPIR-V are not interchangeable). This is a known, currently-unfixed limitation — a proper fix would key the cache by `fileName + "." + apiSuffix + ".bin"` or similar. See `tasks/todo.md` Task 10/11 notes for how this was diagnosed.

- **Camera controls are commented out, not missing by omission.** `Camera::NormalCameraControls`/`OrbitCameraControls` have their entire `Input::` polling logic commented out in `Camera::OnUpdate` and within the control functions themselves, along with the `Math::Frustum`/`Math::AABB`-based frustum culling (`IsInsideFrustum`, `m_Frustum`). `FrustumCulling` is still a public bool member with no effect. This looks like it was disabled during the input/math-library churn that's part of this same refactor rather than a finished, input-free camera.

- **`FindShaderFromHash` is a stub.** It's declared with a real signature (taking a hash and a `hashGenerator` callback) but the body just returns `{nullptr, 0}` unconditionally — a half-finished feature, not currently functional.

- **ImGui rendering does not currently exist in the tree.** `ImGuiRenderPass`/`ImGui_NVRHI` (the donut-derived, nvrhi-based ImGui renderer this doc previously described) was deleted along with `DeviceManager`/`IRenderPass`. `Application::OnImGui` is disabled (commented out in `Application::Render()`). Re-implementing ImGui rendering against `RHI::Device`/`RHI::CommandList`/`RHI::Surface` — recovering the deleted logic (font atlas upload, dynamically-grown vertex/index buffers, per-texture `BindingSetHandle` cache, PSO) from git history as reference — is tracked as Phase 6 in `tasks/plan.md`. Until then, the Editor's window presents a cleared/triangle-rendered frame but no UI panels.

### ImGui Integration

`Engine/src/ImGui/` (`imgui_impl_glfw.cpp`/`.h`, `imgui_impl_vulkan.cpp`/`.h`) contains Dear
ImGui's own official backend files — not Zephyr code, but copied in (with the project's MIT
header prepended) and built as part of the engine. They are functionally identical to the
reference copies under `Engine/Vendor/ImGui/backends/`. Neither is currently wired into the
running application: `imgui_impl_vulkan.cpp` is Vulkan-only, which would defeat the purpose of
an nvrhi-based, API-agnostic renderer (the deleted `ImGui_NVRHI` reimplemented ImGui rendering
directly against nvrhi for exactly this reason, and its replacement — Phase 6 — will do the
same). `imgui_impl_glfw.cpp`'s input-handling half has no such conflict and remains a candidate
for wiring up GLFW input into whatever ImGui renderer Phase 6 produces.

## Dependencies

**Renderer depends on:**
- **nvrhi** (`Engine/Vendor/nvrhi`) — `ShaderFactory` is written directly against nvrhi types (`nvrhi::IDevice`, `nvrhi::ShaderHandle`, etc.); `Renderer` goes through `RHI::Device`/`RHI::Surface`/`RHI::CommandList` instead of touching nvrhi directly.
- **slang** (`Library["slang"]` in `Dependencies.lua`, resolved from `%{VULKAN_SDK}/Lib/slang.lib` — Slang ships inside this project's Vulkan SDK install) — `ShaderFactory`'s compiler backend.
- `Zephyr::RHI` (`RHI::GraphicsAPI`, `RHI::Device`, `RHI::Surface`, `RHI::CommandList`) — see `docs/rhi.md`.
- `Zephyr::Window`, `Zephyr::Modules::IModule`, `Zephyr::Input`, `Zephyr::FileSystem::IFileSystem` — `Renderer`/`Camera`/`ShaderFactory` depend on these Engine submodules respectively.

**What depends on Renderer:**
- `Zephyr::Application` (`Engine/src/Zephyr/Core/Application.cpp`) registers `Zephyr::Renderer` as a core `IModule` at startup, with `RendererData::Api` selecting the active `GraphicsAPI`.
- `Editor` (`Editor/src/App.cpp`) interacts with rendering indirectly through `Zephyr::Application`, not through `Renderer`/RHI types directly.
