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
- [x] Build: `Engine.vcxproj` compiles and links to `Engine.lib` cleanly; grepped the build log for `C4715` — zero matches (previously present). No new warnings from the changed files. (A pre-existing, unrelated `MSB3073` postbuild failure copying `assimp.dll` — a duplicated-`Engine\Engine\` path bug in `Build-Engine.lua`'s postbuildcommands, present before this task and untouched by it — still fails the *link-then-postbuild* step; flagged to the user below, not fixed here per scope discipline)
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
- [x] Build: regenerated `Engine.vcxproj` via premake (new files aren't picked up by an already-generated `.vcxproj` — premake's glob is baked in at generation time, not live), then clean-rebuilt `Engine.vcxproj`; `CommandBuffer.cpp` compiles and `Engine.lib` links cleanly (same pre-existing, unrelated `assimp.dll` postbuild `MSB3073` noted in Task 1/2 remains)
- [x] Manual check: `grep -r CommadBuffer Engine/` returns no results

**Dependencies:** Task 2 (needs `Device::GetNvrhiDevice()`)

**Files likely touched:**
- `Engine/src/Zephyr/RHI/CommadBuffer.h` (deleted)
- `Engine/src/Zephyr/RHI/CommandBuffer.h` (new)
- `Engine/src/Zephyr/RHI/CommandBuffer.cpp` (new)

**Estimated scope:** S (rename + 2 files)

---

## Checkpoint: Foundation
- [ ] `Engine` and `Editor` build clean (Debug, Vulkan default, still non-functional beyond construction)
- [ ] No MSVC C4715 warnings
- [ ] **Human review before starting Phase 1**

---

## Phase 1: Vulkan vertical slice

### Task 4: Port Vulkan device creation into RHI::VulkanDevice
**Description:** Port the working logic from
`Renderer/Platform/Vulkan/VulkanDeviceManager.cpp` (`createInstance`, `installDebugCallback`,
`pickPhysicalDevice`, `findQueueFamilies`, `createDevice`) into `RHI::VulkanDevice`. Keep the
same required/optional instance & device extension sets and the debug-report-callback setup —
this is a port, not a redesign (see plan.md's Vulkan extension-drift risk). `VulkanDevice`'s
constructor should end with a fully created `vk::Instance`, `vk::PhysicalDevice`, `vk::Device`,
graphics/present queues, and `m_NvrhiDevice`/`m_ValidationLayer` populated via
`nvrhi::vulkan::createDevice(...)`.

**Acceptance criteria:**
- [ ] `VulkanDevice` constructor creates a real `vk::Instance` (with validation layer in Debug), enumerates and picks a physical device, creates a logical device with graphics+present queues
- [ ] `nvrhi::vulkan::createDevice()` succeeds and `GetNvrhiDevice()` (Task 2) returns a non-null handle
- [ ] `DefaultMessageCallback` (already in `Device.h`) is wired as the nvrhi message callback, same as `VulkanDeviceManager` does

**Verification:**
- [ ] Build: `Engine` compiles and links against `nvrhi::vulkan`/Vulkan SDK
- [ ] Manual check: run Editor in Debug, confirm log output shows Vulkan instance/device creation info and no `CORE_ERROR`/`CORE_CRITICAL` from `DefaultMessageCallback`

**Dependencies:** Task 2

**Files likely touched:**
- `Engine/src/Zephyr/RHI/Vulkan/VulkanDevice.h`
- `Engine/src/Zephyr/RHI/Vulkan/VulkanDevice.cpp`

**Estimated scope:** M (2 files, largest single port in this phase)

---

### Task 5: Implement RHI::VulkanSurface
**Description:** Create `RHI::VulkanSurface : Surface`, porting `createWindowSurface`,
`createSwapChain`/`destroySwapChain`, `BeginFrame`/`Present`, and the `SwapChainImage`
(`vk::Image` + `nvrhi::TextureHandle`) pattern from `VulkanDeviceManager`. `VulkanDevice::CreateSurface(window)`
constructs it (needs the window's native `HWND`/GLFW handle → `glfwCreateWindowSurface`, same
as the old code). Implement `Surface`'s existing interface (`GetCurrentFrameIndex`, `GetWidth`,
`GetHeight`, `Resize`) plus whatever `Present`/acquire methods Task 6's render loop needs (add
to `Surface.h` if the current interface is insufficient — it currently has no present/acquire
methods at all).

**Acceptance criteria:**
- [ ] `VulkanDevice::CreateSurface()` returns a working `VulkanSurface`, not `Ref<Surface>()`
- [ ] Swapchain is created at the window's current size with `c_FramesInFlight` images
- [ ] `Resize()` correctly recreates the swapchain (port `ResizeSwapChain`'s destroy+recreate pattern)
- [ ] `Surface.h` gains whatever acquire/present methods are needed (e.g. `AcquireNextImage`, `Present`) — extend the interface, don't bypass it

**Verification:**
- [ ] Build: `Engine` compiles
- [ ] Manual check: run Editor, confirm swapchain images are created (log or debugger check), resize the window without a crash

**Dependencies:** Task 4

**Files likely touched:**
- `Engine/src/Zephyr/RHI/Surface.h` (extend interface)
- `Engine/src/Zephyr/RHI/Vulkan/VulkanSurface.h` (new)
- `Engine/src/Zephyr/RHI/Vulkan/VulkanSurface.cpp` (new)
- `Engine/src/Zephyr/RHI/Vulkan/VulkanDevice.cpp` (wire `CreateSurface`)

**Estimated scope:** M (4 files)

---

### Task 6: Wire Renderer to clear and present
**Description:** `Renderer::Initialize()` currently only calls `Device::Create()` and ignores
`m_Data.Window` entirely (see `docs/renderer.md` Design Notes). Fix that: create the `Surface`
from the window, create a `CommandList` (Task 3), and in `Renderer::Render()` (currently empty)
acquire the next image, record a clear-to-color command, submit, and present.

**Acceptance criteria:**
- [ ] `Renderer::Initialize()` calls `m_DeviceHandle->CreateSurface(m_Data.Window)` and stores the result
- [ ] `Renderer::Render(float)` acquires the swapchain image, clears it to a fixed color via `CommandList`, submits, presents
- [ ] `Renderer` reacts to window resize (hook into whatever resize signal `Window`/`ModuleManager` already exposes, or poll size each frame — pick the smaller change and note which)

**Verification:**
- [ ] Build: `Editor` builds and links
- [ ] Manual check: launch Editor, confirm the window shows a solid clear color (not black-by-absence-of-clear, not a crash) for at least a few seconds, resize the window, close cleanly

**Dependencies:** Task 5

**Files likely touched:**
- `Engine/src/Zephyr/Renderer/Renderer.h`
- `Engine/src/Zephyr/Renderer/Renderer.cpp`

**Estimated scope:** S (2 files)

---

## Checkpoint: Vulkan renders
- [ ] Editor opens a window and shows a solid clear color via Vulkan end-to-end
- [ ] Resize and close do not crash or leak validation errors
- [ ] **Human review before starting Phase 2**

---

## Phase 2: Slang → NVRHI shader pipeline

### Task 7: Adapt ShaderFactory to RHI::Device
**Description:** `ShaderFactory` already does the real work (slang compilation to
DXBC/DXIL/SPIR-V based on `nvrhi::GraphicsAPI`, `.bin` caching, `nvrhi::ShaderHandle` creation)
but its constructor takes a raw `nvrhi::DeviceHandle`, not a `Ref<RHI::Device>`. Add a
construction path (overload, or just document/call-site pattern) that goes
`RHI::Device::GetNvrhiDevice()` (Task 2) → `ShaderFactory(device->GetNvrhiDevice(), fs, path)`.
Use `ToNVRHI()` (Task 2) anywhere `RHI::GraphicsAPI` needs converting for shader target
selection — confirm `ShaderFactory::CompileShader`'s existing `switch (m_Device->getGraphicsAPI())`
still does the right thing given a real nvrhi Vulkan device from Task 4.

**Acceptance criteria:**
- [ ] `Renderer` (or a new small owner) can construct a `ShaderFactory` directly from its `Ref<RHI::Device>` with no manual unwrapping at each call site beyond one `GetNvrhiDevice()` call
- [ ] `ShaderFactory::CompileShader` targets SPIR-V correctly when given the Vulkan `RHI::Device`'s nvrhi device (`m_Device->getGraphicsAPI() == nvrhi::GraphicsAPI::VULKAN`)

**Verification:**
- [ ] Build: `Engine` compiles
- [ ] Unit/manual check: construct a `ShaderFactory` in a throwaway call site, confirm `getGraphicsAPI()` reports `VULKAN`

**Dependencies:** Task 4 (needs a real nvrhi device to report a real `getGraphicsAPI()`)

**Files likely touched:**
- `Engine/src/Zephyr/Renderer/ShaderFactory.h`
- `Engine/src/Zephyr/Renderer/ShaderFactory.cpp`

**Estimated scope:** XS–S (1-2 files, mostly wiring — core logic already exists)

---

### Task 8: Minimal slang shader pair + triangle PSO
**Description:** Add a minimal `.slang` vertex+pixel shader pair (e.g. hardcoded fullscreen or
3-vertex triangle, no vertex buffer needed if using `SV_VertexID` tricks — match whatever's
simplest given `ShaderFactory`'s existing API) under a shader assets path the `IFileSystem`
base path can resolve. Wire `Renderer` to compile it via `ShaderFactory::CreateAutoShader` (or
`CreateShader`), build a minimal `nvrhi::GraphicsPipelineHandle`, and replace Task 6's plain
clear with clear+draw of the triangle.

**Acceptance criteria:**
- [ ] A `.slang` file with a vertex and pixel entry point exists in the repo's shader asset path
- [ ] `ShaderFactory::CompileShader` successfully compiles it to SPIR-V and writes a `.bin` cache file next to the source
- [ ] `Renderer` creates a graphics PSO from the compiled shaders and draws a visible triangle each frame

**Acceptance criteria (cache verification):**
- [ ] Deleting the `.bin` and re-running recompiles it; running a second time without deleting reuses the cache (add a log line or breakpoint to confirm `GetByteCode` hits the cache, not `CompileShader`)

**Verification:**
- [ ] Build: `Editor` builds and links against `slang.lib`
- [ ] Manual check: launch Editor, see a colored triangle on screen via Vulkan

**Dependencies:** Task 7

**Files likely touched:**
- `Engine/src/Zephyr/Renderer/Renderer.h`
- `Engine/src/Zephyr/Renderer/Renderer.cpp`
- new `.slang` shader asset file(s)

**Estimated scope:** M (2-3 files + new asset)

---

## Checkpoint: Shader pipeline proven
- [ ] Editor renders a colored triangle on Vulkan via a `.slang` source compiled through `ShaderFactory` to SPIR-V
- [ ] `.bin` cache correctly reused on second run
- [ ] **Human review before starting Phase 3**

---

## Phase 3: D3D11 vertical slice

### Task 9: Port D3D11 device creation into RHI::D3D11Device
**Description:** Port `D3D11DeviceManager::CreateInstanceInternal`/`CreateDevice` (DXGI factory,
adapter enumeration/selection, `D3D11CreateDevice`, `nvrhi::d3d11::createDevice`) into a new
`RHI::D3D11Device : Device`, mirroring the shape `VulkanDevice` now has (Task 4). Wire
`Device::Create(GraphicsAPI::D3D11)` (Task 2's stub) to actually construct it.

**Acceptance criteria:**
- [ ] `Device::Create(GraphicsAPI::D3D11)` returns a working `D3D11Device`
- [ ] `nvrhi::d3d11::createDevice()` succeeds; `GetNvrhiDevice()` returns non-null
- [ ] Adapter selection/logging mirrors `D3D11DeviceManager::GetAdapterName`

**Verification:**
- [ ] Build: `Engine` compiles and links `nvrhi::d3d11`/D3D11 SDK libs
- [ ] Manual check: with `GraphicsAPI::D3D11` set in `Application.cpp`, launch Editor, confirm device-creation log output and no D3D11 debug-layer errors

**Dependencies:** Task 2 (Phase 1/2 not required — can start once Foundation checkpoint passes; sequenced after Phase 2 here only to keep review load manageable)

**Files likely touched:**
- `Engine/src/Zephyr/RHI/D3D11/D3D11Device.h` (new)
- `Engine/src/Zephyr/RHI/D3D11/D3D11Device.cpp` (new)
- `Engine/src/Zephyr/RHI/Device.cpp` (wire factory)

**Estimated scope:** M (3 files)

---

### Task 10: Implement RHI::D3D11Surface
**Description:** Port `D3D11DeviceManager::CreateSwapChain`/`CreateRenderTarget`/
`ReleaseRenderTarget`/`Present`/`ResizeSwapChain` into `RHI::D3D11Surface : Surface`. D3D11 has
a single back buffer (unlike Vulkan's `c_FramesInFlight`), matching
`D3D11DeviceManager::GetBackBufferCount() == 1` — keep that same single-buffer shape rather than
forcing D3D11 into the multi-image `Surface` pattern from Task 5.

**Acceptance criteria:**
- [ ] `D3D11Device::CreateSurface()` returns a working `D3D11Surface` backed by a real DXGI swapchain sized to the window
- [ ] `Resize()` releases and recreates the render target correctly (port `ResizeSwapChain`)

**Verification:**
- [ ] Build: `Engine` compiles
- [ ] Manual check: resize the window under D3D11, no crash, render target recreated

**Dependencies:** Task 9

**Files likely touched:**
- `Engine/src/Zephyr/RHI/D3D11/D3D11Surface.h` (new)
- `Engine/src/Zephyr/RHI/D3D11/D3D11Surface.cpp` (new)

**Estimated scope:** M (2 files)

---

### Task 11: Verify triangle renders via D3D11
**Description:** Switch `Application.cpp`'s hardcoded `RendererData::Api` to
`GraphicsAPI::D3D11` and confirm Task 6/8's clear+triangle render loop works unmodified (aside
from whatever `CommandList`/PSO code, if any, turns out to be nvrhi-abstracted-enough to need no
backend-specific branches — that's the point of the RHI layer). `ShaderFactory` should now
compile the same `.slang` source to DXBC automatically via its existing
`getGraphicsAPI()`-based switch.

**Acceptance criteria:**
- [ ] With `GraphicsAPI::D3D11` set, Editor renders the same triangle as Vulkan (color/position match)
- [ ] No `Renderer.cpp`/`Renderer.h` code changes were needed beyond the `GraphicsAPI` value itself — if any were needed, they indicate a leaky abstraction; note and fix before closing this task

**Verification:**
- [ ] Build: `Editor` builds against DXBC/D3D11 shader target
- [ ] Manual check: visually compare Vulkan vs. D3D11 output side by side

**Dependencies:** Task 10

**Files likely touched:**
- `Engine/src/Zephyr/Core/Application.cpp` (temporary/permanent API switch, per plan.md open question)

**Estimated scope:** XS (verification task, ideally 0-1 files changed)

---

## Checkpoint: D3D11 renders
- [ ] Switching the `GraphicsAPI` value is the only change needed to move from Vulkan to D3D11
- [ ] Resize and close do not crash on either backend
- [ ] **Human review before starting Phase 4**

---

## Phase 4: D3D12 vertical slice (new code — no in-tree or vendored reference)

### Task 12: Implement RHI::D3D12Device
**Description:** No `D3D12DeviceManager` exists anywhere in this repo or the vendored nvrhi to
port from (confirmed via search — see plan.md risks). Build `RHI::D3D12Device : Device` from
scratch: DXGI 1.4+ factory, adapter enumeration (same `IDXGIAdapter`/`DXGI_ADAPTER_DESC` pattern
as `D3D11Device`, Task 9), `D3D12CreateDevice`, a direct command queue, and
`nvrhi::d3d12::createDevice()` (see `nvrhi/d3d12.h` for the `DeviceDesc` shape — it needs the
`ID3D12Device*` and the command queue at minimum). Model the class shape on `D3D11Device`, not
on any external reference.

**Acceptance criteria:**
- [ ] `Device::Create(GraphicsAPI::D3D12)` returns a working `D3D12Device`
- [ ] A `ID3D12CommandQueue` (direct queue) is created and passed into `nvrhi::d3d12::createDevice`
- [ ] `GetNvrhiDevice()` returns non-null and `getGraphicsAPI() == nvrhi::GraphicsAPI::D3D12`

**Verification:**
- [ ] Build: `Engine` compiles and links `nvrhi::d3d12`/D3D12 SDK libs (confirm `NVRHI-D3D12`'s `thirdparty/DirectX-Headers` include requirement, noted in `docs/vendor.md`, is satisfied)
- [ ] Manual check: with `GraphicsAPI::D3D12` set, launch Editor, confirm device/queue creation log output, enable the D3D12 debug layer and confirm no validation errors on startup

**Dependencies:** Task 2; Task 9 recommended first as a structural template (not a hard dependency)

**Files likely touched:**
- `Engine/src/Zephyr/RHI/D3D12/D3D12Device.h` (new)
- `Engine/src/Zephyr/RHI/D3D12/D3D12Device.cpp` (new)
- `Engine/src/Zephyr/RHI/Device.cpp` (wire factory)

**Estimated scope:** L (3 files, but the largest single unknown in this plan — budget extra time; if it exceeds one session, split command-queue/fence setup into its own task before starting)

---

### Task 13: Implement RHI::D3D12Surface
**Description:** Build a flip-model DXGI swapchain (`DXGI_SWAP_EFFECT_FLIP_DISCARD`,
`c_FramesInFlight` buffers) with per-frame fence synchronization (D3D12 requires explicit
GPU/CPU sync that D3D11's driver handled implicitly) — this is the part of D3D12 with the least
overlap with `D3D11Surface`'s shape. Wrap each swapchain buffer as an `nvrhi::TextureHandle` via
`nvrhi::d3d12`'s texture-from-resource path (mirrors how `VulkanSurface`, Task 5, wraps
`vk::Image`s).

**Acceptance criteria:**
- [ ] `D3D12Device::CreateSurface()` returns a working `D3D12Surface` with `c_FramesInFlight` buffered frames
- [ ] Present correctly waits on the appropriate fence before reusing a buffer (no validation errors, no visible tearing/corruption under resize)
- [ ] `Resize()` waits for GPU idle, releases buffers, recreates swapchain, re-wraps buffers

**Verification:**
- [ ] Build: `Engine` compiles
- [ ] Manual check: resize repeatedly under D3D12 with the debug layer on, confirm no validation errors, no crash

**Dependencies:** Task 12

**Files likely touched:**
- `Engine/src/Zephyr/RHI/D3D12/D3D12Surface.h` (new)
- `Engine/src/Zephyr/RHI/D3D12/D3D12Surface.cpp` (new)

**Estimated scope:** L (2 files, fence/sync logic is inherently fiddly — if it drags, split fence management into a small helper class as its own sub-task)

---

### Task 14: Verify triangle renders via D3D12
**Description:** Same as Task 11 but for D3D12: switch `Application.cpp`'s `GraphicsAPI` value,
confirm the existing `Renderer`/`ShaderFactory` code renders the same triangle unmodified via
the DXIL path.

**Acceptance criteria:**
- [ ] With `GraphicsAPI::D3D12` set, Editor renders the same triangle as Vulkan/D3D11
- [ ] No `Renderer.cpp`/`Renderer.h` changes needed beyond the `GraphicsAPI` value

**Verification:**
- [ ] Build: `Editor` builds against DXIL/D3D12 shader target
- [ ] Manual check: visually compare all three backends' output

**Dependencies:** Task 13

**Files likely touched:**
- `Engine/src/Zephyr/Core/Application.cpp`

**Estimated scope:** XS (verification task)

---

## Checkpoint: All three backends render
- [ ] Vulkan, D3D11, D3D12 all render the identical triangle by only changing the `GraphicsAPI` value
- [ ] No backend-specific branches exist in `Renderer.cpp`/`ShaderFactory` call sites
- [ ] **Human review before deleting the legacy stack**

---

## Phase 5: Delete the orphaned legacy stack

### Task 15: Delete DeviceManager + subclasses + IRenderPass + ImGuiRenderPass
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

**Acceptance criteria:**
- [ ] All five files listed above are deleted, plus their now-empty `Platform/D3D11`, `Platform/Vulkan`, `RenderPasses` directories
- [ ] `grep -r "DeviceManager\|IRenderPass\|ImGuiRenderPass"` across `Engine/` and `Editor/` returns zero matches
- [ ] `docs/rhi.md` and `docs/renderer.md` updated to reflect: one device stack, three working backends, slang shader pipeline in place

**Verification:**
- [ ] Build: full solution builds clean on all three `GraphicsAPI` values with these files removed
- [ ] Manual check: re-run the Task 6/11/14 visual checks once more post-deletion to confirm nothing was silently depending on the deleted files

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

## Checkpoint: Legacy stack removed
- [ ] Zero references to `DeviceManager`/`D3D11DeviceManager`/`VulkanDeviceManager`/`IRenderPass`/`ImGuiRenderPass` remain
- [ ] Full solution builds clean on all three `GraphicsAPI` values
- [ ] All three backends independently verified rendering the same triangle via slang-compiled shaders
- [ ] **Human review before starting Phase 6** (Editor UI renders nothing at this point — expected, not a regression to chase)

---

## Phase 6: Re-add ImGui rendering against the new RHI

### Task 16: Port ImGui draw-data rendering onto RHI::Device
**Description:** Task 15 deleted `ImGuiRenderPass`/`ImGui_NVRHI` along with `DeviceManager`. Its
rendering logic (font atlas upload, dynamically-growing vertex/index buffers, per-texture
`BindingSetHandle` cache, PSO setup, draw-command translation from `ImDrawData`) was real and
working — only its plumbing to `DeviceManager`/`IRenderPass` was orphaned. Recover that logic via
`git show <pre-Task-15-commit>:Engine/src/Zephyr/Renderer/RenderPasses/ImGuiRenderPass.cpp` (and
`.h`) and port it into a new class (e.g. `Zephyr::ImGuiRenderer`) that takes a `Ref<RHI::Device>`
and a `RHI::CommandList` instead of a `DeviceManager*`/`IRenderPass` base. `ImGui::CreateContext()`
still happens once at construction, same as before.

**Acceptance criteria:**
- [ ] New renderer class builds the ImGui font atlas texture, vertex/index buffers, and PSO via `RHI::Device`'s nvrhi handle (`GetNvrhiDevice()`) and `ShaderFactory` (Task 7/8's path) instead of raw `nvrhi::DeviceHandle` plumbing through `DeviceManager`
- [ ] `Render(ImDrawData*)`-equivalent records draw commands into a `RHI::CommandList` (Task 3) and submits against the current `RHI::Surface` framebuffer
- [ ] Per-texture `BindingSetHandle` caching behavior is preserved (don't regress to rebuilding binding sets every frame)
- [ ] DPI-aware font rescaling (`DisplayScaleChanged`) is either preserved from the ported logic or explicitly dropped with a one-line note in the code — don't silently lose it

**Verification:**
- [ ] Build: `Engine`/`Editor` compile and link
- [ ] Manual check: launch Editor, confirm ImGui's demo window (or a minimal test window) renders correctly on all three `GraphicsAPI` backends, with no visible texture/binding corruption

**Dependencies:** Checkpoint after Phase 5 (Task 15); Task 7/8 (`ShaderFactory` wired to `RHI::Device`)

**Files likely touched:**
- new `Engine/src/Zephyr/Renderer/ImGuiRenderer.h`/`.cpp` (or similar; naming is an implementation choice)
- `Engine/src/Zephyr/Renderer/Renderer.h`/`.cpp` (own and drive the new renderer)

**Estimated scope:** M–L (2-3 files, but most of the hard logic is a recovery-and-adapt port, not new design — if binding-set caching and buffer growth turn out to need real rework against the new API, split that into its own follow-up task rather than letting this one balloon)

---

### Task 17: Wire GLFW input into ImGui, enable OnImGui
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

**Acceptance criteria:**
- [ ] Keyboard and mouse input reaches ImGui (`ImGuiIO::AddKeyEvent`/`AddMousePosEvent`/etc. or the equivalent `imgui_impl_glfw`-pattern calls), sourced from `Window`'s GLFW callbacks
- [ ] `Application::Render()` calls `OnImGui(deltaTime)` unconditionally (no longer commented out)
- [ ] `Editor`'s existing panels (`HierarchyPanel`, `InfoPanel`, etc. per `docs/editor.md`) become interactive — clicking/typing in them works

**Verification:**
- [ ] Build: `Editor` compiles
- [ ] Manual check: launch Editor, click and type into at least two different panels, confirm ImGui responds (selection changes, text input works), on all three `GraphicsAPI` backends

**Dependencies:** Task 16

**Files likely touched:**
- `Engine/src/Zephyr/Window/Window.h`/`.cpp` (GLFW callback registration)
- `Engine/src/Zephyr/Core/Application.cpp` (uncomment `OnImGui` call)

**Estimated scope:** M (2 files, input wiring is fiddly but mechanical)

---

## Checkpoint: Complete
- [ ] Zero references to `DeviceManager`/`D3D11DeviceManager`/`VulkanDeviceManager`/`IRenderPass`/`ImGuiRenderPass` (the deleted originals) remain
- [ ] Full solution builds clean on all three `GraphicsAPI` values
- [ ] All three backends independently verified rendering the same triangle via slang-compiled shaders
- [ ] Editor panels render and accept input on all three `GraphicsAPI` backends via the new ImGui-on-RHI path
- [ ] `docs/` updated, no longer describing a "mid-flight" RHI
- [ ] Ready for review / merge
