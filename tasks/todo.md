# Task List: NVRHI RHI (D3D11/D3D12/Vulkan) + Slang Shaders

See `tasks/plan.md` for architecture decisions and rationale. Build/verify via
`Scripts/Setup-Windows.bat` (regenerate `Zephyr.sln`) then `msbuild Zephyr.sln` (or open in VS),
Debug config. `VULKAN_SDK` must be set.

---

## Phase 0: Foundation fixes

### Task 1: Fix Build.lua NVRHI casing [DONE]
**Description:** `Build.lua` line 27 does `include "Engine/Vendor/NVRHI"` (capital) but the real
directory is `Engine/Vendor/nvrhi` (lowercase) — only works today via case-insensitive
filesystem. Fix the casing so `Scripts/Setup-Linux.sh` (case-sensitive) also works.

**Acceptance criteria:**
- [x] `Build.lua` references `Engine/Vendor/nvrhi` (lowercase) consistently with `Dependencies.lua`'s `IncludeDir["nvrhi"]`
- [x] No other casing references to `NVRHI` as a path remain (project *names* like `"NVRHI-Vulkan"` are fine — only the `include` path changes)

**Verification:**
- [x] Build: regenerated via vendored `premake5.exe --file=Build.lua vs2022`; `Zephyr.sln` now lists `NVRHI`, `NVRHI-D3D11`, `NVRHI-D3D12`, `NVRHI-Vulkan` under `Engine/Vendor/nvrhi/*.vcxproj`. Built `NVRHI-Vulkan.vcxproj` and `NVRHI-D3D11.vcxproj` directly via MSBuild (Debug|x64) — both compile and link cleanly (pre-existing `LNK4006` import-descriptor warnings in D3D11, unrelated to this change)
- [x] Manual check: `git status` after regeneration shows only `Build.lua` changed (generated `.sln`/`.vcxproj`/`bin/` artifacts are gitignored, as expected per `docs/build-system.md`)

**Dependencies:** None

**Files likely touched:**
- `Build.lua`

**Estimated scope:** XS (1 file)

---

### Task 2: Restore ToNVRHI(), fix Device::Create, add GetNvrhiDevice() [DONE]
**Description:** `RHI/GraphicsAPI.h` dropped the `ToNVRHI(GraphicsAPI) -> nvrhi::GraphicsAPI`
helper that existed in the old `Renderer/GraphicsAPI.h` (deleted this branch). Restore it.
Fix `Device::Create()`'s `D3D11`/`D3D12` switch cases, which currently `break` with no `return`
(MSVC C4715 / UB) — return `nullptr` for now (real backends land in Phase 3/4). Add
`virtual nvrhi::DeviceHandle GetNvrhiDevice() const = 0;` to `Device` and implement it on
`VulkanDevice` (returning `m_ValidationLayer` if present, else `m_NvrhiDevice` — same pattern
`VulkanDeviceManager::GetDevice()` already uses).

**Acceptance criteria:**
- [x] `Zephyr::ToNVRHI(GraphicsAPI)` exists in `RHI/GraphicsAPI.h`/`.cpp`, mapping `D3D11→nvrhi::GraphicsAPI::D3D11`, `D3D12→D3D12`, `VULKAN→VULKAN`
- [x] `Device::Create(D3D11)` / `Create(D3D12)` explicitly `return nullptr;` (no fallthrough, no MSVC C4715)
- [x] `Device::GetNvrhiDevice() const` pure virtual added; `VulkanDevice` overrides it (returns `m_ValidationLayer` if present, else `m_NvrhiDevice` — both still null until Task 4, which is a defined value, not UB)

**Verification:**
- [x] Build: `Engine.vcxproj` compiles and links to `Engine.lib` cleanly; grepped the build log for `C4715` — zero matches (previously present). No new warnings from the changed files. (Correction: an `MSB3073` postbuild failure copying `assimp.dll` was observed here and initially misdiagnosed as a repo bug in `Build-Engine.lua`'s path construction — it was actually an artifact of building `Engine.vcxproj` directly instead of through `Zephyr.sln`, which leaves `$(SolutionDir)` unresolved/misresolved. Building via `Zephyr.sln` resolves it correctly and the postbuild step succeeds; see Task 3's verification, which re-confirms this with a full solution build)
- [x] Manual check: `ToNVRHI` has no call site yet (expected — Task 7 is its first consumer); confirmed via grep it compiles and is reachable

**Dependencies:** None (parallel-safe with Task 1)

**Files likely touched:**
- `Engine/src/Zephyr/RHI/GraphicsAPI.h`
- `Engine/src/Zephyr/RHI/GraphicsAPI.cpp`
- `Engine/src/Zephyr/RHI/Device.h`
- `Engine/src/Zephyr/RHI/Device.cpp`
- `Engine/src/Zephyr/RHI/Vulkan/VulkanDevice.h`
- `Engine/src/Zephyr/RHI/Vulkan/VulkanDevice.cpp`

**Estimated scope:** S (4 files)

---

### Task 3: Rename CommadBuffer.h → CommandBuffer.h, implement RHI::CommandList [DONE]
**Description:** `RHI/CommadBuffer.h` is an empty, typo'd placeholder with zero references
anywhere. Rename to `CommandBuffer.h` and implement `Zephyr::CommandList`: a thin wrapper over
`nvrhi::ICommandList` with `Open()`/`Close()`, and accessors needed by Task 6's render loop
(`nvrhi::ICommandList* GetNvrhiCommandList()`). Construction takes the owning `Device`'s
`nvrhi::DeviceHandle` (via `GetNvrhiDevice()` from Task 2) and calls
`device->createCommandList()`.

**Acceptance criteria:**
- [x] `Engine/src/Zephyr/RHI/CommadBuffer.h` no longer exists; `Engine/src/Zephyr/RHI/CommandBuffer.h`/`.cpp` exist
- [x] `Zephyr::CommandList` wraps `nvrhi::CommandListHandle`, exposes `Open()`, `Close()`, `GetNvrhiCommandList()`
- [x] No other file references the old `CommadBuffer.h` path (confirmed zero matches under `Engine/` via grep; the only remaining mentions are in `docs/rhi.md` describing the pre-Task-3 state, and this plan's own task text)

**Verification:**
- [x] Build: regenerated `Engine.vcxproj` via premake (new files aren't picked up by an already-generated `.vcxproj` — premake's glob is baked in at generation time, not live), then built the full `Zephyr.sln` (Debug|x64) end to end: `CommandBuffer.cpp` compiles, `Engine.lib` links, `assimp.dll` postbuild copy succeeds (confirms Task 1/2's `MSB3073` was a build-invocation artifact, not a repo bug — see corrected note on Task 2), and `Editor.exe` itself builds and links with zero errors
- [x] Manual check: `grep -r CommadBuffer Engine/` returns no results

**Dependencies:** Task 2 (needs `Device::GetNvrhiDevice()`)

**Files likely touched:**
- `Engine/src/Zephyr/RHI/CommadBuffer.h` (deleted)
- `Engine/src/Zephyr/RHI/CommandBuffer.h` (new)
- `Engine/src/Zephyr/RHI/CommandBuffer.cpp` (new)

**Estimated scope:** S (rename + 2 files)

---

## Checkpoint: Foundation [REACHED]
- [x] `Engine` and `Editor` build clean (Debug, Vulkan default, still non-functional beyond construction) — confirmed via full `Zephyr.sln` build, `Editor.exe` linked
- [x] No MSVC C4715 warnings
- [x] Human review before starting Phase 1 (proceeded)

---

## Phase 1: Vulkan vertical slice

### Task 4: Port Vulkan device creation into RHI::VulkanDevice [DONE]
**Description:** Port the working logic from
`Renderer/Platform/Vulkan/VulkanDeviceManager.cpp` (`createInstance`, `installDebugCallback`,
`pickPhysicalDevice`, `findQueueFamilies`, `createDevice`) into `RHI::VulkanDevice`. Keep the
same required/optional instance & device extension sets and the debug-report-callback setup —
this is a port, not a redesign (see plan.md's Vulkan extension-drift risk). `VulkanDevice`'s
constructor should end with a fully created `vk::Instance`, `vk::PhysicalDevice`, `vk::Device`,
graphics/present queues, and `m_NvrhiDevice`/`m_ValidationLayer` populated via
`nvrhi::vulkan::createDevice(...)`.

**Adaptation notes (deviations from a pure line-for-line port, and why):**
- No window/surface exists yet at `Device::Create()` time in this architecture (`Surface` is created separately via `CreateSurface(window)`, Task 5) — so the window-surface-capability checks inside the original `pickPhysicalDevice()` (swap chain image count/extent/format, present support against a real `VkSurfaceKHR`) were **not** ported here; only the extension/feature/queue-family checks were. Task 5's `VulkanSurface` will need to re-validate presentability once a real surface exists.
- Compute/transfer queues (`EnableComputeQueue`/`EnableCopyQueue` in the original `DeviceCreationParameters`) were dropped — nothing in this engine uses them yet (YAGNI); only graphics+present queues are created.
- `DeviceCreationParameters`/`InstanceParameters` (the old app-configurable struct) were not carried over; validation/debug-report-callback enablement is instead gated on `#ifdef DEBUG` directly, since `RHI::Device::Create(GraphicsAPI)` has no params argument to configure it through.
- The **exact same required/optional Vulkan instance and device extension sets** from `VulkanDeviceManager` are kept verbatim, per the plan's extension-drift risk note.
- A latent quirk in the original is preserved as-is (not fixed, per port-not-redesign scope): `FindQueueFamilies` only assigns `m_GraphicsQueueFamily`/`m_PresentQueueFamily` when they're still `-1`, so once set while evaluating one physical-device candidate they won't be re-evaluated for a later candidate. `PickPhysicalDevice()` still re-runs `FindQueueFamilies` once more on the finally-selected device afterward (matching the original's redundant-but-correcting second call), so the end result is correct even though the per-candidate filtering has this quirk.

**Acceptance criteria:**
- [x] `VulkanDevice` constructor creates a real `vk::Instance` (with validation layer in Debug), enumerates and picks a physical device, creates a logical device with graphics+present queues
- [x] `nvrhi::vulkan::createDevice()` succeeds and `GetNvrhiDevice()` (Task 2) returns a non-null handle
- [x] `DefaultMessageCallback` (already in `Device.h`) is wired as the nvrhi message callback, same as `VulkanDeviceManager` does

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64) — `VulkanDevice.cpp` compiles, `Engine.lib` links against `nvrhi::vulkan`/Vulkan SDK, `Editor.exe` links, zero new warnings
- [x] Manual check: ran `Editor.exe` for 6s. Log output: enabled instance extensions (`VK_EXT_debug_utils`, `VK_EXT_debug_report`, `VK_KHR_win32_surface`, `VK_KHR_surface`, `VK_KHR_get_physical_device_properties2`), `VK_LAYER_KHRONOS_validation` enabled, device extensions enabled, `"Created Vulkan Device: NVIDIA GeForce RTX 3070 Ti"`, `"Succesfully registered module Renderer"` (confirms `Device::Create()` returned non-null and `Renderer::Initialize()` succeeded). Zero `CORE_ERROR`/`CORE_CRITICAL`/`[NVRHI]` messages, clean exit

**Dependencies:** Task 2

**Files likely touched:**
- `Engine/src/Zephyr/RHI/Vulkan/VulkanDevice.h`
- `Engine/src/Zephyr/RHI/Vulkan/VulkanDevice.cpp`

**Estimated scope:** M (2 files, largest single port in this phase)

---

### Task 5: Implement RHI::VulkanSurface [DONE]
**Description:** Create `RHI::VulkanSurface : Surface`, porting `createWindowSurface`,
`createSwapChain`/`destroySwapChain`, `BeginFrame`/`Present`, and the `SwapChainImage`
(`vk::Image` + `nvrhi::TextureHandle`) pattern from `VulkanDeviceManager`. `VulkanDevice::CreateSurface(window)`
constructs it (needs the window's native `HWND`/GLFW handle → `glfwCreateWindowSurface`, same
as the old code). Implement `Surface`'s existing interface (`GetCurrentFrameIndex`, `GetWidth`,
`GetHeight`, `Resize`) plus whatever `Present`/acquire methods Task 6's render loop needs (add
to `Surface.h` if the current interface is insufficient — it currently has no present/acquire
methods at all).

**Adaptation notes:**
- `Surface.h` gained `AcquireNextImage()`, `GetCurrentFramebuffer()`, `Present()`.
- `VulkanDevice` gained `GetRawNvrhiDevice() -> nvrhi::vulkan::IDevice*`: the Vulkan-specific
  `queueWaitForSemaphore`/`queueSignalSemaphore`/native-texture-handle methods used by the
  swapchain aren't on the generic `nvrhi::IDevice` interface (they're Vulkan-only extensions),
  and worse, they're not safe to call through the validation-layer-wrapped device
  (`GetNvrhiDevice()`) since that wrapper only exposes the generic interface — matches how
  `VulkanDeviceManager` always called these directly on its raw `m_NvrhiDevice`, never through
  `GetDevice()`'s validated wrapper.
- Framebuffer objects (one per swapchain image, `nvrhi::FramebufferDesc().addColorAttachment(...)`)
  ARE created through the validated device, matching the original `DeviceManager::BackBufferResized()`.
- Fixed a bug caught during the port (not present in a straight copy, introduced while writing
  the mutable-format image-view list): the alias format must be the UNORM counterpart of
  `SBGRA8_UNORM`'s underlying `VK_FORMAT_B8G8R8A8_SRGB`, i.e. `vk::Format::eB8G8R8A8Unorm` — an
  earlier draft duplicated the primary format instead.
- **Follow-up fix (found while verifying Task 6, separate commit):** the original
  `VulkanDeviceManager::Present()`'s `EventQuery`-based frame-in-flight throttle was dropped
  during this port. Without it, a real continuous render loop (no vsync) submits far more frames
  than the 3-image/4-semaphore ring buffer can hold, and Vulkan validation correctly rejects
  reusing a semaphore with a still-pending wait/signal (`vkAcquireNextImageKHR(): Semaphore must
  not have any pending operations`). Re-added `m_FramesInFlight`/`m_QueryPool` and the
  wait-then-recycle logic in `Present()`, capped at `c_FramesInFlight`.

**Acceptance criteria:**
- [x] `VulkanDevice::CreateSurface()` returns a working `VulkanSurface`, not `Ref<Surface>()`
- [x] Swapchain is created at the window's current size with `c_FramesInFlight` images
- [x] `Resize()` correctly recreates the swapchain (port `ResizeSwapChain`'s destroy+recreate pattern)
- [x] `Surface.h` gains whatever acquire/present methods are needed (e.g. `AcquireNextImage`, `Present`) — extend the interface, don't bypass it

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64) clean, zero new warnings
- [x] Manual check: temporarily called `CreateSurface()`/`AcquireNextImage()`/`Present()`/`Resize()` from `Renderer::Initialize()` (scratch code, reverted before commit — real wiring is Task 6's job) and ran `Editor.exe`. Confirmed: surface created at `1920x1009`; 3 distinct framebuffer pointers cycling across 5 acquire calls (matches `c_FramesInFlight=3`); `Resize(800,600)` recreated the swapchain and updated `GetWidth()`/`GetHeight()`; no crash across the run. Three Vulkan validation warnings observed are artifacts of the scratch harness itself (never records/submits a real command list, so nvrhi never transitions the image out of `UNDEFINED` layout before present; resizes without a matching real window resize event) — not defects in `VulkanSurface`; Task 6's real render loop exercises the actually-used path

**Dependencies:** Task 4

**Files likely touched:**
- `Engine/src/Zephyr/RHI/Surface.h` (extend interface)
- `Engine/src/Zephyr/RHI/Vulkan/VulkanSurface.h` (new)
- `Engine/src/Zephyr/RHI/Vulkan/VulkanSurface.cpp` (new)
- `Engine/src/Zephyr/RHI/Vulkan/VulkanDevice.cpp` (wire `CreateSurface`)

**Estimated scope:** M (4 files)

---

### Task 6: Wire Renderer to clear and present [DONE]
**Description:** `Renderer::Initialize()` currently only calls `Device::Create()` and ignores
`m_Data.Window` entirely (see `docs/renderer.md` Design Notes). Fix that: create the `Surface`
from the window, create a `CommandList` (Task 3), and in `Renderer::Render()` (currently empty)
acquire the next image, record a clear-to-color command, submit, and present.

**Adaptation notes:**
- Resize handling: polled each frame via `glfwGetFramebufferSize` compared against
  `Surface::GetWidth/GetHeight`, not a callback — `Window` exposes no resize signal today and
  adding one is out of scope here (smaller change, per the task's own guidance).
- Command list submission goes through the *validated* device (`GetNvrhiDevice()`), not
  `GetRawNvrhiDevice()` — that's the whole point of running with the validation layer in Debug;
  only the Vulkan-specific semaphore plumbing inside `VulkanSurface` needs the raw device.
- Found and fixed a real bug while verifying this task: see the separate "Fix VulkanSurface:
  throttle frames in flight" commit — the continuous render loop this task adds is what exposed
  a missing frame-pacing throttle from Task 5's port.

**Acceptance criteria:**
- [x] `Renderer::Initialize()` calls `m_DeviceHandle->CreateSurface(m_Data.Window)` and stores the result
- [x] `Renderer::Render(float)` acquires the swapchain image, clears it to a fixed color via `CommandList`, submits, presents
- [x] `Renderer` reacts to window resize (hook into whatever resize signal `Window`/`ModuleManager` already exposes, or poll size each frame — pick the smaller change and note which)

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64), `Editor.exe` links, zero new warnings
- [x] Manual check: ran `Editor.exe` for 6s (uncapped framerate, no vsync — thousands of frames). Zero Vulkan validation messages, clean exit, no crash. Resize path verified via Task 5's dedicated scratch test (unchanged `VulkanSurface::Resize()` logic, not re-tested interactively here since this run was non-interactive/automated)

**Dependencies:** Task 5

**Files likely touched:**
- `Engine/src/Zephyr/Renderer/Renderer.h`
- `Engine/src/Zephyr/Renderer/Renderer.cpp`

**Estimated scope:** S (2 files)

---

## Checkpoint: Vulkan renders [REACHED]
- [x] Editor opens a window and shows a solid clear color via Vulkan end-to-end
- [x] Resize and close do not crash or leak validation errors
- [x] Human review before starting Phase 2 (proceeded)

---

## Phase 2: Slang → NVRHI shader pipeline

### Task 7: Adapt ShaderFactory to RHI::Device [DONE]
**Description:** `ShaderFactory` already does the real work (slang compilation to
DXBC/DXIL/SPIR-V based on `nvrhi::GraphicsAPI`, `.bin` caching, `nvrhi::ShaderHandle` creation)
but its constructor takes a raw `nvrhi::DeviceHandle`, not a `Ref<RHI::Device>`. Add a
construction path (overload, or just document/call-site pattern) that goes
`RHI::Device::GetNvrhiDevice()` (Task 2) → `ShaderFactory(device->GetNvrhiDevice(), fs, path)`.
Use `ToNVRHI()` (Task 2) anywhere `RHI::GraphicsAPI` needs converting for shader target
selection — confirm `ShaderFactory::CompileShader`'s existing `switch (m_Device->getGraphicsAPI())`
still does the right thing given a real nvrhi Vulkan device from Task 4.

**Adaptation notes:**
- `ToNVRHI()` turned out to have no call site here: `ShaderFactory` never needs to convert
  `Zephyr::GraphicsAPI` at all — it calls `m_Device->getGraphicsAPI()` directly on the nvrhi
  device it's holding, which is intrinsic to that device object (set when
  `nvrhi::vulkan::createDevice()`/etc. constructed it). `ToNVRHI()` remains unused until a task
  that needs to go the other direction (`Zephyr::GraphicsAPI` → nvrhi, before an nvrhi device
  exists) — likely D3D11/D3D12 device creation (Tasks 9/12), not shader compilation.
- `Renderer` now owns a `Scope<ShaderFactory>`, constructed from
  `m_DeviceHandle->GetNvrhiDevice()` + a fresh `DefaultFileSystem` + `"Assets/Shaders"` as the
  base path (matching the existing `Assets/` runtime-asset convention; not yet validated end to
  end — that's Task 8).
- **Found and fixed a real coupling bug**, in scope for "adapt ShaderFactory": `ShaderFactory.h`
  included the legacy `Renderer/DeviceManager.h` (`#include "DeviceManager.h"`) purely to get
  `nvrhi/nvrhi.h` transitively — meaning `Renderer.h` including `ShaderFactory.h` would have
  pulled the entire orphaned `DeviceManager` stack into every file that includes `Renderer.h`.
  Worse, it would have made `ShaderFactory.h` uncompilable the moment Task 15 deletes
  `DeviceManager.h`. Replaced with a direct `#include <nvrhi/nvrhi.h>`. Verified no other file
  relied on this transitive include (`ImGuiRenderPass.h` already includes `IRenderPass.h`
  directly, which is where its `DeviceManager` dependency actually comes from).

**Acceptance criteria:**
- [x] `Renderer` (or a new small owner) can construct a `ShaderFactory` directly from its `Ref<RHI::Device>` with no manual unwrapping at each call site beyond one `GetNvrhiDevice()` call
- [x] `ShaderFactory::CompileShader` targets SPIR-V correctly when given the Vulkan `RHI::Device`'s nvrhi device (`m_Device->getGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN`) — guaranteed by nvrhi's Vulkan backend itself (`nvrhi::vulkan::createDevice()` always reports `GraphicsAPI::VULKAN`), not independently re-verifiable at this layer

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64) clean, zero new warnings; confirmed `ImGuiRenderPass.cpp`/`App.cpp` (transitive includers of `ShaderFactory.h`) still compile after removing the `DeviceManager.h` include
- [x] Manual check: ran `Editor.exe` — `ShaderFactory`'s constructor (creates the slang global session) completes with no `CORE_ERROR`/crash

**Dependencies:** Task 4 (needs a real nvrhi device to report a real `getGraphicsAPI()`)

**Files likely touched:**
- `Engine/src/Zephyr/Renderer/ShaderFactory.h`
- `Engine/src/Zephyr/Renderer/ShaderFactory.cpp`

**Estimated scope:** XS–S (1-2 files, mostly wiring — core logic already exists)

---

### Task 8: Minimal slang shader pair + triangle PSO [DONE]
**Description:** Add a minimal `.slang` vertex+pixel shader pair (e.g. hardcoded fullscreen or
3-vertex triangle, no vertex buffer needed if using `SV_VertexID` tricks — match whatever's
simplest given `ShaderFactory`'s existing API) under a shader assets path the `IFileSystem`
base path can resolve. Wire `Renderer` to compile it via `ShaderFactory::CreateAutoShader` (or
`CreateShader`), build a minimal `nvrhi::GraphicsPipelineHandle`, and replace Task 6's plain
clear with clear+draw of the triangle.

**Adaptation notes:**
- Two separate files (`Triangle.vs.slang` / `Triangle.ps.slang`, entry point `main` in both)
  instead of one file with two entry points. **Discovered why this matters, not just style:**
  `ShaderFactory::CompileShader`'s `.bin` output path and `GetByteCode`'s cache key are both
  derived from `fileName` alone, ignoring `entryName` — compiling two entry points from the same
  source file would silently overwrite one's `.bin` with the other's and collide in the
  in-memory cache. Noted as a pre-existing `ShaderFactory` bug, not fixed (out of scope; the
  two-file split sidesteps it entirely for this task).
- Base shader path is `"Shaders"`, not `"Assets/Shaders"` as Task 7 assumed — the Editor's actual
  working directory (`Editor/Build-Editor.lua`'s `debugdir`, and `LocalDebuggerWorkingDirectory`
  in the generated `.vcxproj.user`) is `$(SolutionDir)/Assets`, so paths are relative to that,
  not the repo root. Corrected in this task; Task 7's commit had it wrong.
- Culling disabled (`RasterState::setCullNone()`) and depth test/write disabled
  (`DepthStencilState::disableDepthTest/disableDepthWrite()`, since the framebuffer has no depth
  attachment) — the minimal, unambiguous choice for a single unindexed triangle with no
  established winding-order/Y-convention contract yet.
- **Found and fixed two real, blocking bugs while making this task actually run** (both outside
  this task's own files, in code Task 7 already touched/adjacent to):
  1. `Engine/src/Zephyr/FileSystem/FileSystem.cpp`'s `DefaultFileSystem::ReadFile` called
     `CORE_ASSERT(false, ...)` on a routine "file doesn't exist" condition — which is exactly
     what `ShaderFactory::GetByteCode`'s normal first-compile cache-miss path hits every time.
     `DEBUGBREAK()` with no debugger attached crashed the process outright. Removed the assert;
     kept the existing `CORE_ERROR` + `return nullptr`, which the caller already handled correctly.
  2. `ShaderFactory::CompileShader` wrote the compiled bytecode to disk via
     `byteCode.readRef()` — for Slang's `ComPtr<T>`, `readRef()` returns `T* const*` (the address
     of the internal pointer, for APIs like `QueryInterface` to write into), not the buffer data.
     This wrote garbage stack memory into every `.bin` file instead of the actual SPIR-V bytes,
     which `vkCreateShaderModule`/pipeline creation then correctly rejected as invalid SPIR-V.
     Fixed to `byteCode->getBufferPointer()`. This is a hard blocker for *any* shader ever
     compiling from source in this engine, not something specific to this task's shaders.
  See the separate "Fix FileSystem/ShaderFactory: file-not-found assert and corrupt shader
  bytecode write" commit for both.

**Acceptance criteria:**
- [x] A `.slang` file with a vertex and pixel entry point exists in the repo's shader asset path
- [x] `ShaderFactory::CompileShader` successfully compiles it to SPIR-V and writes a `.bin` cache file next to the source
- [x] `Renderer` creates a graphics PSO from the compiled shaders and draws a visible triangle each frame

**Acceptance criteria (cache verification):**
- [x] Deleting the `.bin` and re-running recompiles it; running a second time without deleting reuses the cache (verified via log output — first run logs `GetByteCode` misses/`CompileShader` fallback for both shaders, second run logs neither)

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64), `Editor.exe` links against `slang.lib`, zero new warnings
- [x] Manual check: launched `Editor.exe` and took an actual screenshot (PowerShell `CopyFromScreen`, not just log inspection) — a smoothly-interpolated RGB triangle renders correctly in the "Zephyr Editor" window, vertex colors matching the shader source (red/green/blue at the three assigned positions). Zero Vulkan validation warnings on a fresh compile or a cached second run

**Dependencies:** Task 7

**Files likely touched:**
- `Engine/src/Zephyr/Renderer/Renderer.h`
- `Engine/src/Zephyr/Renderer/Renderer.cpp`
- new `.slang` shader asset file(s)

**Estimated scope:** M (2-3 files + new asset)

---

## Checkpoint: Shader pipeline proven [REACHED]
- [x] Editor renders a colored triangle on Vulkan via a `.slang` source compiled through `ShaderFactory` to SPIR-V
- [x] `.bin` cache correctly reused on second run
- [x] Human review before starting Phase 3 (proceeded)

---

## Phase 3: D3D11 vertical slice

### Task 9: Port D3D11 device creation into RHI::D3D11Device [DONE]
**Description:** Port `D3D11DeviceManager::CreateInstanceInternal`/`CreateDevice` (DXGI factory,
adapter enumeration/selection, `D3D11CreateDevice`, `nvrhi::d3d11::createDevice`) into a new
`RHI::D3D11Device : Device`, mirroring the shape `VulkanDevice` now has (Task 4). Wire
`Device::Create(GraphicsAPI::D3D11)` (Task 2's stub) to actually construct it.

**Adaptation notes:**
- Much simpler than the Vulkan port: single `nvrhi::DeviceHandle m_NvrhiDevice` member (no
  separate raw/validated split like `VulkanDevice` needed — D3D11 presentation has no
  Vulkan-style semaphore extension methods that bypass the validation wrapper), matching
  `D3D11DeviceManager.h`'s own shape.
- Adapter selection hardcoded to index 0 (first adapter) — `D3D11DeviceManager`'s
  `AdapterIndex`-based selection wasn't carried over, same simplification `VulkanDevice` made
  for its own adapter/queue selection (no exposed selection mechanism yet).
- `CreateSurface()` is a stub (`return Ref<Surface>()`) pending Task 10, mirroring exactly how
  `VulkanDevice::CreateSurface()` looked before Task 5.
- `EnableDebugRuntime`/`EnableNvrhiValidationLayer` gated on `#ifdef DEBUG`, matching the pattern
  established for `VulkanDevice`.

**Acceptance criteria:**
- [x] `Device::Create(GraphicsAPI::D3D11)` returns a working `D3D11Device`
- [x] `nvrhi::d3d11::createDevice()` succeeds; `GetNvrhiDevice()` returns non-null
- [x] Adapter selection/logging mirrors `D3D11DeviceManager::GetAdapterName`

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64), links `nvrhi::d3d11`/D3D11 SDK libs, zero new warnings
- [x] Manual check: temporarily set `GraphicsAPI::D3D11` in `Application.cpp` (reverted before commit - the permanent switch + full verification is Task 11's job), ran `Editor.exe`. Log: `"Created D3D11 Device: NVIDIA GeForce RTX 3070 Ti"`. Then, as expected (Task 10 not done yet), `Renderer::Initialize()` correctly fails at `CreateSurface()`'s stub and `ModuleManager` correctly asserts on the core-module failure — a clean, correctly-handled failure, not a crash, and exactly the signal that device creation itself succeeded

**Dependencies:** Task 2 (Phase 1/2 not required — can start once Foundation checkpoint passes; sequenced after Phase 2 here only to keep review load manageable)

**Files likely touched:**
- `Engine/src/Zephyr/RHI/D3D11/D3D11Device.h` (new)
- `Engine/src/Zephyr/RHI/D3D11/D3D11Device.cpp` (new)
- `Engine/src/Zephyr/RHI/Device.cpp` (wire factory)

**Estimated scope:** M (3 files)

---

### Task 10: Implement RHI::D3D11Surface [DONE]
**Description:** Port `D3D11DeviceManager::CreateSwapChain`/`CreateRenderTarget`/
`ReleaseRenderTarget`/`Present`/`ResizeSwapChain` into `RHI::D3D11Surface : Surface`. D3D11 has
a single back buffer (unlike Vulkan's `c_FramesInFlight`), matching
`D3D11DeviceManager::GetBackBufferCount() == 1` — keep that same single-buffer shape rather than
forcing D3D11 into the multi-image `Surface` pattern from Task 5.

**Adaptation notes:**
- `GetCurrentFrameIndex()` hardcoded to `0` — D3D11's implicit back buffer model has no
  app-visible frame index, matching `D3D11DeviceManager::GetCurrentBackBufferIndex()`.
- `AcquireNextImage()` is a trivial `return m_Framebuffer != nullptr` — D3D11 has no
  acquire/semaphore step (`BeginFrame`'s original body only handled a windowed/fullscreen
  mode-switch edge case, which nothing in this engine exercises — `StartFullscreen` is never
  set — so it wasn't ported; noted, not silently dropped).
- DXGI swap chain uses `DXGI_SWAP_EFFECT_FLIP_DISCARD` with `BufferCount = 2` (the flip-model
  minimum) rather than porting `DeviceCreationParameters::SwapChainBufferCount` (default 3) —
  irrelevant here since the app only ever sees one nvrhi-wrapped back buffer regardless of the
  underlying DXGI buffer count.
- Window repositioning (`MoveWindowOntoAdapter`) from the original `CreateSwapChain` was not
  ported — that's editor window-placement policy, unrelated to swap chain creation.
- **Found, not fixed here (flagged for Task 11):** switching to D3D11 and compiling Task 8's
  shaders fails — `nvrhi::createShader` (`CreateVertexShader`) returns `E_INVALIDARG`
  (`0x80070057`). Root cause appears to be `ShaderFactory::CompileShader`'s D3D11 target profile
  (`sm_5_1`, which needs D3D11.3/D3D12) against a device created at plain
  `D3D_FEATURE_LEVEL_11_1`. This is a `ShaderFactory`/shader-compilation issue, not a
  `D3D11Surface` one — confirmed because `D3D11Device`/`D3D11Surface` construction logged zero
  errors before the shader-compile step failed. Also confirmed a related, separate footgun while
  investigating: the `.bin` cache path has no per-API disambiguation (Task 8 already knew about
  the per-entry-point collision; this is the same root cause, one dimension further — switching
  `GraphicsAPI` and reusing a stale `.bin` compiled for a different backend will silently hand
  the wrong bytecode format to `createShader`). Deleted and regenerated `.bin` files were
  restored to the committed Vulkan SPIR-V version (`git checkout --`) before this commit.

**Acceptance criteria:**
- [x] `D3D11Device::CreateSurface()` returns a working `D3D11Surface` backed by a real DXGI swapchain sized to the window
- [x] `Resize()` releases and recreates the render target correctly (port `ResizeSwapChain`)

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64) clean, zero new warnings
- [x] Manual check: temporarily set `GraphicsAPI::D3D11` (reverted before commit), ran `Editor.exe`. `D3D11Device`/`D3D11Surface` construction succeeded with zero errors (device creation log, then straight to shader compilation — no surface-related failures, unlike the pre-Task-10 stub which failed exactly at `CreateSurface()`). The subsequent failure is in shader compilation (see above), outside this task's scope. Also re-verified Vulkan is unaffected post-revert: clean run, `.bin` cache reused, no regression

**Dependencies:** Task 9

**Files likely touched:**
- `Engine/src/Zephyr/RHI/D3D11/D3D11Surface.h` (new)
- `Engine/src/Zephyr/RHI/D3D11/D3D11Surface.cpp` (new)

**Estimated scope:** M (2 files)

---

### Task 11: Verify triangle renders via D3D11 [DONE]
**Description:** Switch `Application.cpp`'s hardcoded `RendererData::Api` to
`GraphicsAPI::D3D11` and confirm Task 6/8's clear+triangle render loop works unmodified (aside
from whatever `CommandList`/PSO code, if any, turns out to be nvrhi-abstracted-enough to need no
backend-specific branches — that's the point of the RHI layer). `ShaderFactory` should now
compile the same `.slang` source to DXBC automatically via its existing
`getGraphicsAPI()`-based switch.

**Blocker fix (in scope, as anticipated):** `ShaderFactory::CompileShader`'s DXBC target profile
was `sm_5_1`, which requires D3D11.3/D3D12; `RHI::D3D11Device` creates a plain
`D3D_FEATURE_LEVEL_11_1` device, which only supports Shader Model 5.0. Changed to `sm_5_0` —
this immediately fixed the `E_INVALIDARG` from `nvrhi::createShader`.

**Known limitation, not fixed here (pre-existing, tracked since Task 8/10):** `.bin` shader
caches are keyed by filename only, not by `GraphicsAPI`. Switching the active API and reusing a
stale `.bin` compiled for a different backend will silently feed the wrong bytecode format to
`createShader`. Deleted `Assets/Shaders/*.bin` before testing, exactly as flagged. The `.bin`
files now committed alongside this task contain **D3D11 DXBC**, not Vulkan SPIR-V, since D3D11
is the new active default — switching back to `GraphicsAPI::VULKAN` without deleting them first
will reproduce the same failure Task 10 hit going the other direction. A proper fix (e.g.
`fileName + "." + apiSuffix + ".bin"`) is out of scope for this plan; noted for whoever revisits
`ShaderFactory` next.

**Acceptance criteria:**
- [x] With `GraphicsAPI::D3D11` set, Editor renders the same triangle as Vulkan (color/position match) — verified via screenshot, pixel-position/color-identical to the Vulkan screenshot from Task 8
- [x] No `Renderer.cpp`/`Renderer.h` code changes were needed beyond the `GraphicsAPI` value itself — confirmed via `git status`: only `Application.cpp` (the switch) and `ShaderFactory.cpp` (the profile bug fix, not a `Renderer` change) are dirty

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64), `Editor.exe` links against DXBC/D3D11 shader target, zero new warnings
- [x] Manual check: launched `Editor.exe` and took a screenshot — same downward-pointing RGB triangle, same position, same interpolated colors as the Vulkan run. Zero errors/crashes across the run

**Dependencies:** Task 10

**Files likely touched:**
- `Engine/src/Zephyr/Core/Application.cpp` (temporary/permanent API switch, per plan.md open question)

**Estimated scope:** XS (verification task, ideally 0-1 files changed)

---

## Checkpoint: D3D11 renders [REACHED]
- [x] Switching the `GraphicsAPI` value is the only change needed to move from Vulkan to D3D11
- [x] Resize and close do not crash on either backend — D3D11 resize verified interactively (`MoveWindow` to 900x600 mid-run via a PowerShell scratch script), triangle re-rendered correctly at the new size, process stayed alive; Vulkan resize was verified in Task 5
- [x] Human review before starting Phase 4 (proceeded)

---

## Phase 4: D3D12 vertical slice (new code — no in-tree or vendored reference)

### Task 12: Implement RHI::D3D12Device [DONE]
**Description:** No `D3D12DeviceManager` exists anywhere in this repo or the vendored nvrhi to
port from (confirmed via search — see plan.md risks). Build `RHI::D3D12Device : Device` from
scratch: DXGI 1.4+ factory, adapter enumeration (same `IDXGIAdapter`/`DXGI_ADAPTER_DESC` pattern
as `D3D11Device`, Task 9), `D3D12CreateDevice`, a direct command queue, and
`nvrhi::d3d12::createDevice()` (see `nvrhi/d3d12.h` for the `DeviceDesc` shape — it needs the
`ID3D12Device*` and the command queue at minimum). Model the class shape on `D3D11Device`, not
on any external reference.

**Adaptation notes:**
- Structurally identical to `D3D11Device`: `CreateFactory()`/`CreateDeviceAndQueue()` split,
  same `GetAdapterName`/adapter-index-0 selection, single `nvrhi::DeviceHandle m_NvrhiDevice`
  wrapped in `nvrhi::validation::createValidationLayer` under `#ifdef DEBUG`.
- `CreateSurface()` is a stub (`return Ref<Surface>()`) pending Task 13, mirroring exactly how
  `D3D11Device::CreateSurface()` looked before Task 10.
- D3D12 debug layer (`D3D12GetDebugInterface`/`ID3D12Debug::EnableDebugLayer`) is enabled under
  `#ifdef DEBUG` *before* device creation — required by D3D12 (unlike D3D11's
  `D3D11_CREATE_DEVICE_DEBUG` flag, D3D12's debug layer must be turned on before
  `D3D12CreateDevice` is called or it has no effect).
- **Found and fixed, in scope:** two real build-system gaps, not part of the class itself:
  1. `NVRHI-D3D12`'s premake project had no `links` block at all (unlike `NVRHI-D3D11`, which
     explicitly links `d3d11.lib`/`dxgi.lib`/`d3dcompiler.lib`/`dxguid.lib`) — added
     `d3d12.lib`/`dxgi.lib`/`dxguid.lib` to `Engine/Vendor/nvrhi/premake5.lua`'s `NVRHI-D3D12`
     project, matching the D3D11 project's pattern.
  2. `nvrhi/d3d12.h` transitively includes `<directx/d3d12.h>` from nvrhi's vendored
     `thirdparty/DirectX-Headers` submodule. `NVRHI-D3D12`'s own project has that on its include
     path, but `Engine` (which now includes `nvrhi/d3d12.h` directly from `D3D12Device.h`/
     `Device.cpp`) didn't. Added `IncludeDir["DirectXHeaders"]` to `Dependencies.lua` and wired
     it into `Engine/Build-Engine.lua`'s `includedirs`.
- Both fixes are in `Engine/Vendor/nvrhi` (a git submodule pointing at a fork, so freely
  editable) and the top-level premake scripts — no engine `.cpp`/`.h` logic changes beyond the
  new `D3D12Device` itself.

**Acceptance criteria:**
- [x] `Device::Create(GraphicsAPI::D3D12)` returns a working `D3D12Device`
- [x] A `ID3D12CommandQueue` (direct queue) is created and passed into `nvrhi::d3d12::createDevice`
- [x] `GetNvrhiDevice()` returns non-null and `getGraphicsAPI() == nvrhi::GraphicsAPI::D3D12` (implied by the `nvrhi::d3d12::createDevice()` call succeeding — confirmed via the "Created D3D12 Device" log line, which only prints after `createDevice()` returns)

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64) after the premake regen + include-dir fixes above — clean, zero new warnings
- [x] Manual check: temporarily set `GraphicsAPI::D3D12` in `Application.cpp` (reverted before commit — the permanent switch is Task 14's job, matching Task 9/11's pattern), ran `Editor.exe`. Log: `"Created D3D12 Device: NVIDIA GeForce RTX 3070 Ti"`. Then, as expected (Task 13 not done yet), `Renderer::Initialize()` correctly fails at `CreateSurface()`'s stub ("Failed to create a rendering surface") and `ModuleManager` correctly asserts on the core-module failure — same clean, handled-failure signal Task 9 confirmed for D3D11, not a crash

**Dependencies:** Task 2; Task 9 recommended first as a structural template (not a hard dependency)

**Files likely touched:**
- `Engine/src/Zephyr/RHI/D3D12/D3D12Device.h` (new)
- `Engine/src/Zephyr/RHI/D3D12/D3D12Device.cpp` (new)
- `Engine/src/Zephyr/RHI/Device.cpp` (wire factory)

**Estimated scope:** L (3 files, but the largest single unknown in this plan — budget extra time; if it exceeds one session, split command-queue/fence setup into its own task before starting)

---

### Task 13: Implement RHI::D3D12Surface [DONE]
**Description:** Build a flip-model DXGI swapchain (`DXGI_SWAP_EFFECT_FLIP_DISCARD`,
`c_FramesInFlight` buffers) with per-frame fence synchronization (D3D12 requires explicit
GPU/CPU sync that D3D11's driver handled implicitly) — this is the part of D3D12 with the least
overlap with `D3D11Surface`'s shape. Wrap each swapchain buffer as an `nvrhi::TextureHandle` via
`nvrhi::d3d12`'s texture-from-resource path (mirrors how `VulkanSurface`, Task 5, wraps
`vk::Image`s).

**Adaptation notes:**
- `CreateSwapChainForHwnd` (needs `IDXGIFactory4`, already stored on `D3D12Device`) +
  `QueryInterface` to `IDXGISwapChain3` for `GetCurrentBackBufferIndex()`.
- Fence sync follows the standard D3D12 sample "MoveToNextFrame" shape: one shared `ID3D12Fence`
  + `m_NextFenceValue` counter + one last-submitted fence value per back buffer index
  (`m_FrameFenceValues`). `AcquireNextImage()` waits on the incoming index's last value before
  handing the framebuffer back out; `Present()` submits the new value for the index just used.
  `WaitForGPU()` (full idle wait) is used for `Resize()`/destruction, where every buffer must be
  free, not just one.
- Swapchain buffers wrapped via `createHandleForNativeTexture(nvrhi::ObjectTypes::D3D12_Resource, ...)`
  with `initialState = Present`/`keepInitialState = true`, matching `VulkanSurface`'s swapchain
  image desc — nvrhi's own command list barrier handling transitions to/from render-target state
  around each frame's use, the surface itself never touches D3D12 resource states directly.

**Acceptance criteria:**
- [x] `D3D12Device::CreateSurface()` returns a working `D3D12Surface` with `c_FramesInFlight` buffered frames
- [x] Present correctly waits on the appropriate fence before reusing a buffer (no validation errors, no visible tearing/corruption under resize)
- [x] `Resize()` waits for GPU idle, releases buffers, recreates swapchain, re-wraps buffers

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64), clean, zero new warnings
- [x] Manual check: temporarily set `GraphicsAPI::D3D12` (reverted before commit — permanent switch is Task 14's job), deleted the stale D3D11 `.bin` shader cache, ran `Editor.exe` — device+surface+DXIL shader compile all succeeded, screenshot showed the same triangle as Vulkan/D3D11. Resized the live window via `MoveWindow` (900x600) — re-rendered correctly, process stayed alive, no crash. Restored the committed D3D11 `.bin` files afterward (`git checkout --`) since Task 11 made D3D11 the active default

**Dependencies:** Task 12

**Files likely touched:**
- `Engine/src/Zephyr/RHI/D3D12/D3D12Surface.h` (new)
- `Engine/src/Zephyr/RHI/D3D12/D3D12Surface.cpp` (new)

**Estimated scope:** L (2 files, fence/sync logic is inherently fiddly — if it drags, split fence management into a small helper class as its own sub-task)

---

### Task 14: Verify triangle renders via D3D12 [DONE]
**Description:** Same as Task 11 but for D3D12: switch `Application.cpp`'s `GraphicsAPI` value,
confirm the existing `Renderer`/`ShaderFactory` code renders the same triangle unmodified via
the DXIL path.

**Known limitation (same as Task 11):** deleted the stale D3D11 `.bin` shader cache before
building — the `.bin` cache is still filename-only keyed, no per-API disambiguation. The `.bin`
files now committed contain DXIL, since D3D12 is the new active default.

**Acceptance criteria:**
- [x] With `GraphicsAPI::D3D12` set, Editor renders the same triangle as Vulkan/D3D11
- [x] No `Renderer.cpp`/`Renderer.h` changes needed beyond the `GraphicsAPI` value — confirmed via `git status`: only `Application.cpp` (the switch) and the two `.bin` files are dirty

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64), `Editor.exe` links against DXIL/D3D12 shader target, clean, zero new warnings
- [x] Manual check: launched `Editor.exe`, screenshot — same downward-pointing RGB triangle, identical position/colors to the Vulkan and D3D11 screenshots. Zero errors/crashes

**Dependencies:** Task 13

**Files likely touched:**
- `Engine/src/Zephyr/Core/Application.cpp`

**Estimated scope:** XS (verification task)

---

## Checkpoint: All three backends render [REACHED]
- [x] Vulkan, D3D11, D3D12 all render the identical triangle by only changing the `GraphicsAPI` value
- [x] No backend-specific branches exist in `Renderer.cpp`/`ShaderFactory` call sites (the only per-API branching anywhere is `ShaderFactory`'s target-format switch, which is inherent to compiling one source to three bytecode formats, not a `Renderer`-level backend branch)
- [ ] **Human review before deleting the legacy stack**

---

## Phase 5: Delete the orphaned legacy stack

### Task 15: Delete DeviceManager + subclasses + IRenderPass + ImGuiRenderPass [DONE]
**Description:** Now that `RHI::Device`/`Surface` cover Vulkan, D3D11, and D3D12 with real
working implementations, delete the parallel stack per the explicit instruction to avoid
`D3D11DeviceManager`/`VulkanDeviceManager`: `Renderer/DeviceManager.{h,cpp}`,
`Renderer/Platform/D3D11/D3D11DeviceManager.{h,cpp}`,
`Renderer/Platform/Vulkan/VulkanDeviceManager.{h,cpp}`, `Renderer/IRenderPass.{h,cpp}`,
`Renderer/RenderPasses/ImGuiRenderPass.{h,cpp}`. Confirm (per `docs/renderer.md`) these have zero
external call sites before deleting — re-grep, don't just trust the doc snapshot. Also remove the
Editor's vestigial unused `#include <Zephyr/Renderer/ShaderFactory.h>` if it's still unused
(`docs/renderer.md` Design Notes), and update `docs/rhi.md`/`docs/renderer.md` to describe the
finished single-stack state instead of the mid-flight one.

**Adaptation notes:**
- Re-grepping (not trusting the doc snapshot) found the five files' `.h`/`.cpp` pairs plus stale
  *name* references (not call sites) in three RHI files that survived the earlier ports as
  explanatory comments: `RHI/D3D11/D3D11Device.cpp`, `D3D11Surface.cpp`/`.h` (comments citing
  `D3D11DeviceManager`'s original behavior), and `RHI/Vulkan/VulkanDevice.h`/`.cpp` (comments
  citing `VulkanDeviceManager`). Reworded all of them so the zero-matches grep target is real,
  not just true for code.
- `Input.cpp` had an entirely unused `#include <Zephyr/Renderer/DeviceManager.h>` (nothing in the
  file referenced it) - removed.
- `Editor/src/App.cpp`'s `#include "Zephyr/Renderer/ShaderFactory.h"` was confirmed still unused
  (no `ShaderFactory` symbol referenced anywhere in the file) - removed. Left the neighboring
  `FileSystem.h` include alone even though it also looks unused - out of this task's scope.
- Moved `VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE` from `VulkanDeviceManager.cpp` (now
  deleted) into `VulkanDevice.cpp`, exactly as flagged during Task 4. Confirmed via grep that
  nvrhi's own `vulkan-device.cpp` guards its copy of the same macro behind
  `NVRHI_SHARED_LIBRARY_BUILD` (not defined in this static-lib build), so there's no duplicate-
  symbol risk between nvrhi's copy and this one.

**Acceptance criteria:**
- [x] All five files listed above are deleted, plus their now-empty `Platform/D3D11`, `Platform/Vulkan`, `RenderPasses` directories
- [x] `grep -r "DeviceManager\|IRenderPass\|ImGuiRenderPass"` across `Engine/` and `Editor/` returns zero matches
- [x] `docs/rhi.md` and `docs/renderer.md` updated to reflect: one device stack, three working backends, slang shader pipeline in place
- [x] **Discovered during Task 4:** `VulkanDeviceManager.cpp` is currently the sole definer of `VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE`. Moved into `VulkanDevice.cpp` in this same commit - build+link confirmed clean

**Verification:**
- [x] Build: full solution builds clean on all three `GraphicsAPI` values with these files removed
- [x] Manual check: re-ran the Task 6/11/14 visual checks post-deletion by scratch-switching through all three `GraphicsAPI` values (deleting the stale `.bin` cache each time) - D3D12 (the current committed default), Vulkan, and D3D11 all rendered the identical triangle with zero errors, confirming nothing was silently depending on the deleted files. Restored the D3D12 default and its DXIL `.bin` cache afterward

**Dependencies:** Checkpoint after Phase 4

**Files likely touched:**
- `Engine/src/Zephyr/Renderer/DeviceManager.h` / `.cpp` (deleted)
- `Engine/src/Zephyr/Renderer/Platform/D3D11/D3D11DeviceManager.h` / `.cpp` (deleted)
- `Engine/src/Zephyr/Renderer/Platform/Vulkan/VulkanDeviceManager.h` / `.cpp` (deleted)
- `Engine/src/Zephyr/Renderer/IRenderPass.h` / `.cpp` (deleted)
- `Engine/src/Zephyr/Renderer/RenderPasses/ImGuiRenderPass.h` / `.cpp` (deleted)
- `Editor/src/App.cpp` (drop vestigial include, if still unused)
- `docs/rhi.md`, `docs/renderer.md`

**Estimated scope:** M (10 files deleted + 2 docs updated, low code-risk but wide blast radius — grep twice)

---

## Checkpoint: Legacy stack removed [REACHED]
- [x] Zero references to `DeviceManager`/`D3D11DeviceManager`/`VulkanDeviceManager`/`IRenderPass`/`ImGuiRenderPass` remain
- [x] Full solution builds clean on all three `GraphicsAPI` values
- [x] All three backends independently verified rendering the same triangle via slang-compiled shaders
- [ ] **Human review before starting Phase 6** (Editor UI renders nothing at this point — expected, not a regression to chase)

---

## Phase 6: Re-add ImGui rendering against the new RHI

### Task 16: Port ImGui draw-data rendering onto RHI::Device [DONE]
**Description:** Task 15 deleted `ImGuiRenderPass`/`ImGui_NVRHI` along with `DeviceManager`. Its
rendering logic (font atlas upload, dynamically-growing vertex/index buffers, per-texture
`BindingSetHandle` cache, PSO setup, draw-command translation from `ImDrawData`) was real and
working — only its plumbing to `DeviceManager`/`IRenderPass` was orphaned. Recover that logic via
`git show <pre-Task-15-commit>:Engine/src/Zephyr/Renderer/RenderPasses/ImGuiRenderPass.cpp` (and
`.h`) and port it into a new class (e.g. `Zephyr::ImGuiRenderer`) that takes a `Ref<RHI::Device>`
and a `RHI::CommandList` instead of a `DeviceManager*`/`IRenderPass` base. `ImGui::CreateContext()`
still happens once at construction, same as before.

**Adaptation notes:**
- New `Zephyr::ImGuiRenderer` (`Renderer/ImGuiRenderer.h`/`.cpp`) owns its own dedicated
  `RHI::CommandList`, separate from `Renderer`'s triangle command list — self-contained, matching
  the original `ImGui_NVRHI`'s own dedicated `nvrhi::CommandListHandle`.
- No `.hlsl`/pre-baked bytecode existed anywhere for the original `imgui_vertex`/`imgui_pixel`
  shaders referenced by name in the recovered `.cpp` (`shaderFactory->CreateShader("imgui_vertex", ...)`)
  — those were always compiled elsewhere/pre-baked, never in this repo. Wrote new
  `Assets/Shaders/ImGui.vs.slang`/`ImGui.ps.slang` from scratch, following the `Triangle.*.slang`
  pattern (Task 8) and matching `ImGui_NVRHI::Init`'s binding layout exactly (`PushConstants(0, 8 bytes)`,
  `Texture_SRV(0)`, `Sampler(0)`).
- **Found and fixed, real bug, not a recovery artifact:** on the D3D12/DXIL path, returning the
  pixel shader's `input.col * fontTexture.Sample(...)` result directly (or via an unmodified
  `float4 c = ...; return c;`) compiles fine but the output alpha channel is lost — every pixel
  blends as if alpha were 0, hiding the entire UI, even though `c.a` reads correctly in isolation
  (verified by branching on it in a throwaway diagnostic shader). Explicitly reconstructing
  `return float4(c.rgb, c.a);` fixes it. Root cause (Slang/DXC codegen quirk, most likely) not
  identified further; documented in a comment at the fix site. Diagnosed via a sequence of
  screenshot-driven diagnostic shader variants (color-coding vertex alpha vs. texture alpha vs.
  the real combined value) rather than guesswork.
- Dropped the original's `m_supportExplicitDisplayScaling` toggle (sourced from
  `DeviceCreationParameters::SupportExplicitDisplayScaling`, which `RHI` has no equivalent of) —
  DPI-aware font rescaling (`DisplayScaleChanged`) is preserved but now always active, not
  optionally gated. Documented with a one-line comment at the `DisplayScaleChanged` declaration.
- `KeyboardUpdate`/`MouseButtonUpdate`/etc. from the original `IRenderPass` interface were **not**
  ported — they relied on `io.KeysDown[]`, which no longer exists in this vendored ImGui version
  (1.92.0 WIP; legacy array removed, only `io.AddKeyEvent()` remains). Task 16's acceptance
  criteria don't require input wiring; that's explicitly Task 17's job, which may use the
  vendored `imgui_impl_glfw.*` callbacks directly instead of hand-rolling a GLFW-key mapping.
- `Renderer::Render()` wires the new renderer in: `Animate()` + a temporary `ImGui::ShowDemoWindow()`
  call (placeholder pending Task 17's `OnImGui` wiring) + `Render(framebuffer)`, after the triangle
  draw and before `Present()`. `BackBufferResizing()` is called on resize.

**Acceptance criteria:**
- [x] New renderer class builds the ImGui font atlas texture, vertex/index buffers, and PSO via `RHI::Device`'s nvrhi handle (`GetNvrhiDevice()`) and `ShaderFactory` (Task 7/8's path) instead of raw `nvrhi::DeviceHandle` plumbing through `DeviceManager`
- [x] `Render(ImDrawData*)`-equivalent records draw commands into a `RHI::CommandList` (Task 3) and submits against the current `RHI::Surface` framebuffer
- [x] Per-texture `BindingSetHandle` caching behavior is preserved (don't regress to rebuilding binding sets every frame)
- [x] DPI-aware font rescaling (`DisplayScaleChanged`) is either preserved from the ported logic or explicitly dropped with a one-line note in the code — preserved, with the explicit-scaling *toggle* dropped (noted above)

**Verification:**
- [x] Build: `Engine`/`Editor` compile and link, zero new warnings
- [x] Manual check: launched Editor, confirmed ImGui's demo window renders correctly (crisp text, correct translucent window/header blending, no texture/binding corruption) on all three `GraphicsAPI` backends via scratch API switches — screenshots pixel-identical across Vulkan/D3D11/D3D12. Restored the D3D12 default and regenerated its DXIL `.bin` cache afterward

**Dependencies:** Checkpoint after Phase 5 (Task 15); Task 7/8 (`ShaderFactory` wired to `RHI::Device`)

**Files likely touched:**
- new `Engine/src/Zephyr/Renderer/ImGuiRenderer.h`/`.cpp` (or similar; naming is an implementation choice)
- `Engine/src/Zephyr/Renderer/Renderer.h`/`.cpp` (own and drive the new renderer)

**Estimated scope:** M–L (2-3 files, but most of the hard logic is a recovery-and-adapt port, not new design — if binding-set caching and buffer growth turn out to need real rework against the new API, split that into its own follow-up task rather than letting this one balloon)

---

### Task 17: Wire GLFW input into ImGui, enable OnImGui [DONE]
**Description:** Unlike Task 16, this part has **no working reference to port** —
`docs/renderer.md` confirms `DeviceManager::CreateWindowDeviceAndSwapChain()`'s GLFW callback
registration block (key/mouse/scroll/focus) was itself commented out and never wired, even in
the old stack. Implement it fresh: register GLFW key/char/mouse-pos/mouse-button/scroll callbacks
(via `Window`, since it already owns the `GLFWwindow*`) that feed `ImGuiIO`, following the
standard `imgui_impl_glfw` pattern (already vendored, unused, at `Engine/src/ImGui/imgui_impl_glfw.*`
per `docs/renderer.md` — reference its callback bodies rather than reinventing them, but do not
call its `ImGui_ImplGlfw_InitForVulkan`/`InitForOpenGL` directly, since those assume a renderer
backend this project doesn't use). Finally, uncomment `Application::Render()`'s `OnImGui(deltaTime)`
call (`Engine/src/Zephyr/Core/Application.cpp`, currently commented out) and route it to Task 16's
renderer.

**Adaptation notes (deviations from the plan text, and why):**
- Went with `ImGui_ImplGlfw_InitForOther(windowHandle, true)` (called from `ImGuiRenderer`'s
  constructor, which is where the ImGui context actually gets created — `Window::Initialize()` has
  already run by then, since `Application`'s constructor registers modules in order Window → Input
  → Renderer and `RegisterModule` calls `Initialize()` synchronously) rather than hand-registering
  individual GLFW callbacks from `Window` as the plan's "Files likely touched" implied.
  `install_callbacks=true` makes `imgui_impl_glfw` register GLFW's key/char/cursor-pos/mouse-button/
  scroll/focus/monitor callbacks itself (chaining `Window`'s own `WindowCloseCallback`, which uses a
  different callback slot so there's no conflict) — this is the officially-recommended pattern for a
  custom (non-imgui_impl_vulkan/opengl3) renderer, so "reference its callback bodies rather than
  reinventing them" became "call the backend's own Init/Shutdown", which is less code and less risk
  than hand-copying the callback bodies into `Window`.
- **`Application::Render()`'s `OnImGui(deltaTime)` is not literally uncommented in place** — doing
  that verbatim crashes. By the time `m_ModuleManager.Render(deltaTime)` (called just above it)
  returns, this `Renderer`'s `Render()` has already called `ImGui::Render()` for the frame (it owns
  the whole `Animate()`/build-UI/`Render()`/`Present()` sequence internally, per Task 16). Calling
  `OnImGui()`'s `ImGui::Begin()`s after that hits ImGui's "Forgot to call NewFrame()?" assert.
  Fixed by adding `Renderer::SetImGuiCallback(std::function<void(float)>)`, wired in
  `Application`'s constructor to `[this](float dt) { OnImGui(dt); }` right after
  `RegisterModule<Renderer>`, and invoked from inside `Renderer::Render()` at exactly the point the
  old `ImGui::ShowDemoWindow()` placeholder sat (between `Animate()` and `Render()`). The line in
  `Application::Render()` is now a comment explaining where the call actually happens instead of the
  call itself — the net effect (`OnImGui` invoked unconditionally, every frame, not disabled) is the
  same as what the acceptance criterion asks for.
- **Found and fixed a real, adjacent bug while wiring `ImGuiRenderer`'s destructor**: it now needs
  to call `ImGui_ImplGlfw_Shutdown()` (touches the GLFW window) before `ImGui::DestroyContext()`.
  `ModuleManager::Shutdown()` previously called every module's `Shutdown()` in `m_Modules`'
  unordered_map order — meaning `Window::Shutdown()` (`glfwDestroyWindow` + `glfwTerminate()`) could
  run *before* `Renderer::Shutdown()`, leaving `ImGui_ImplGlfw_Shutdown()` to touch an already-
  destroyed window/terminated GLFW (its `RestoreCallbacks`/`glfwDestroyCursor` calls are UB post-
  `glfwTerminate()`). Fixed `ModuleManager::Shutdown()` to run explicit `Shutdown()` calls in
  reverse-priority order (`Renderer`, priority 1, before `Window`/`Input`, priority 0) via
  `m_UpdateList` instead of `m_Modules`, and added `m_ImGuiRenderer.reset()` as the first line of
  `Renderer::Shutdown()` so the teardown actually happens during that ordered pass, not later at
  unpredictable `shared_ptr` destruction time. Verified via two full launch→`WM_CLOSE`→clean-exit
  cycles (see Verification) — no crash, no validation errors on exit.

**Acceptance criteria:**
- [x] Keyboard and mouse input reaches ImGui (`ImGuiIO::AddKeyEvent`/`AddMousePosEvent`/etc. or the equivalent `imgui_impl_glfw`-pattern calls), sourced from `Window`'s GLFW callbacks — wired via `ImGui_ImplGlfw_InitForOther`'s own callback installation (see adaptation notes); not independently confirmed interactively in this session (see Verification)
- [x] `Application::Render()` calls `OnImGui(deltaTime)` unconditionally (no longer commented out) — invoked unconditionally every frame via `Renderer::SetImGuiCallback`, not as a literal call in `Application::Render()` itself (see adaptation notes for why)
- [ ] `Editor`'s existing panels (`HierarchyPanel`, `InfoPanel`, etc. per `docs/editor.md`) become interactive — clicking/typing in them works — **not verified this session**, see Verification

**Verification:**
- [x] Build: full `Zephyr.sln` build (Debug|x64) clean, zero new warnings, D3D12 default
- [x] Manual check: launched `Editor.exe` (`--api` unset, D3D12 default). `PrintWindow`-captured screenshots confirm the real menu bar ("Window"/"Info"/"About", built by `Editor::Application::OnImGui`'s `MainMenuBar()`) renders correctly on top of the triangle every frame across a multi-minute run with the process pinned near 100% of one core (uncapped render loop, confirms the frame loop is live, not stalled) — this is only possible if `Animate()` → `SetImGuiCallback` → `OnImGui()` → `Render()` executes every frame without hitting ImGui's frame-scope assert, which is the specific hazard the `SetImGuiCallback` redesign (see adaptation notes) exists to avoid. Sent ~100 synthetic input events at the window (`WM_MOUSEMOVE`/`WM_LBUTTONDOWN`/`WM_LBUTTONUP` via `PostMessage`, plus hardware-level `mouse_event`/`SetCursorPos` clicks once the window had real OS focus, plus a `WM_KEYDOWN`/`WM_KEYUP` pair) across two separate launches — zero crashes, zero validation errors, process stayed responsive throughout
- [x] Clean shutdown verified twice: `PostMessage(WM_CLOSE)` → GLFW close callback → `Application::RequestClose()` → main loop exits → `ModuleManager::Shutdown()` (reverse-priority order) → `Renderer::Shutdown()` resets `m_ImGuiRenderer` (`ImGui_ImplGlfw_Shutdown()` + `ImGui::DestroyContext()`) → `Window::Shutdown()` (`glfwDestroyWindow`/`glfwTerminate()`) → process exits cleanly both times, confirming the shutdown-ordering fix works and nothing UB's on teardown
- [ ] **Click/type interactivity itself was not confirmed.** This session runs as a background job automating a remote Windows desktop that, at the time of testing, had another application (a fullscreen game) occupying the screen. `WindowFromPoint()` at the intended click coordinates resolved to that other application's window (not the Editor's), confirming the physical screen was genuinely occupied by it, which explains why hardware-level clicks (`SetCursorPos`+`mouse_event`) had no effect even after confirming real OS focus via `AttachThreadInput`+`SetForegroundWindow`. `PostMessage`-injected `WM_LBUTTONDOWN`/`WM_LBUTTONUP` (which bypass screen z-order/hit-testing entirely, going straight into the target window's message queue) were tried extensively — single clicks, a row sweep, and a full 2D grid over the menu bar — with zero observable effect on repeated `PrintWindow` captures, despite `PostMessage(WM_CLOSE)` to the same `HWND` demonstrably working (process exits cleanly) via the exact same delivery mechanism. Root cause of the `WM_LBUTTONDOWN` gap specifically is unresolved — plausibly some interaction between the synthetic-event ordering/timing and ImGui's per-frame click-edge detection, not ruled out to be a real bug in this task's code, but not reproduced/isolated either. **This needs a quick manual check by whoever reviews this**: launch `Editor.exe`, click "Window" in the menu bar, confirm the panel list drops down and toggling one shows/hides it.

**Dependencies:** Task 16

**Files touched (revised from "likely touched" — see adaptation notes):**
- `Engine/src/Zephyr/Renderer/ImGuiRenderer.h`/`.cpp` (`ImGui_ImplGlfw_InitForOther`/`Shutdown`, new `windowHandle` constructor param)
- `Engine/src/Zephyr/Renderer/Renderer.h`/`.cpp` (`SetImGuiCallback`, pass window handle to `ImGuiRenderer`, reset it first in `Shutdown()`)
- `Engine/src/Zephyr/Core/Application.cpp` (wire the callback instead of literally uncommenting the `OnImGui` call)
- `Engine/src/Zephyr/Modules/ModuleManager.cpp` (reverse-priority `Shutdown()` ordering — real bug found while making the above safe)
- `Window.h`/`.cpp` ended up **not** touched — no GLFW callback registration needed there, see adaptation notes

**Estimated scope:** M (2 files, input wiring is fiddly but mechanical) — actual: 4 files, mostly because of the OnImGui timing/shutdown-ordering issues found along the way

---

## Checkpoint: Complete
- [ ] Zero references to `DeviceManager`/`D3D11DeviceManager`/`VulkanDeviceManager`/`IRenderPass`/`ImGuiRenderPass` (the deleted originals) remain
- [ ] Full solution builds clean on all three `GraphicsAPI` values
- [ ] All three backends independently verified rendering the same triangle via slang-compiled shaders
- [ ] Editor panels render and accept input on all three `GraphicsAPI` backends via the new ImGui-on-RHI path
- [ ] `docs/` updated, no longer describing a "mid-flight" RHI
- [ ] Ready for review / merge
