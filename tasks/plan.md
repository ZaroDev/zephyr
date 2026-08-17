# Implementation Plan: NVRHI-based RHI (D3D11 / D3D12 / Vulkan) + Slang Shader Pipeline

## Overview

`Engine/src/Zephyr/RHI/` is a skeleton: `GraphicsAPI` enum, an abstract `Device`/`Surface`,
and a `VulkanDevice` whose methods are all empty bodies. It is the *only* device path actually
wired into the running application (`Renderer::Initialize()` → `Device::Create()`), but it
draws nothing today — no swapchain, no command list, no shaders.

Sitting alongside it, `Engine/src/Zephyr/Renderer/DeviceManager.h/.cpp` +
`Platform/D3D11/D3D11DeviceManager` + `Platform/Vulkan/VulkanDeviceManager` +
`IRenderPass`/`ImGuiRenderPass` is a **complete, working** device/swapchain/render-loop stack
ported from NVIDIA's donut framework — real Vulkan instance/device/swapchain setup, real D3D11
device/swapchain setup — but it is orphaned: nothing outside `Renderer/` instantiates any of it.

This plan finishes `RHI/` by **porting the working logic out of `DeviceManager`'s Vulkan and
D3D11 subclasses into `RHI::VulkanDevice`/`RHI::D3D11Device`**, **writing `RHI::D3D12Device`
from scratch** (no reference implementation exists anywhere in this tree), wiring a minimal
render loop through `Zephyr::Renderer` so each backend can be visually verified, connecting the
already-functional `ShaderFactory` (slang → DXBC/DXIL/SPIR-V → `nvrhi::ShaderHandle`) to the new
`Device`, and then **deleting** `DeviceManager` + its subclasses + `IRenderPass` +
`ImGuiRenderPass` so only one device abstraction exists in the tree, per the explicit instruction
to avoid `D3D11DeviceManager`/`VulkanDeviceManager` going forward.

D3D12 is the highest-risk piece: nvrhi vendors no example `DeviceManager`, and this repo has no
prior D3D12 code to port — it is genuinely new, modeled on the shape of `D3D11Device` plus
nvrhi's `d3d12.h` API surface.

## Architecture Decisions

- **One device abstraction, not two.** `RHI::Device` becomes the sole owner of a graphics
  device; `DeviceManager` and its subclasses are deleted once each backend reaches parity, not
  kept "for reference." Dead code that still compiles is exactly what confused the last person
  to touch this branch (see `docs/rhi.md`/`docs/renderer.md` Design Notes) — it should not
  persist through this work.
- **`RHI::Surface` gets one concrete class per backend** (`VulkanSurface`, `D3D11Surface`,
  `D3D12Surface`), because swapchain mechanics are fundamentally different per API (VkSwapchainKHR
  vs. DXGI `IDXGISwapChain`) — there is no useful shared base implementation, only the shared
  interface that already exists.
- **`RHI::Device` grows a `GetNvrhiDevice()` accessor** returning `nvrhi::DeviceHandle`. Nothing
  above nvrhi (`ShaderFactory`, future PSO/resource code) can function without this — it's the
  single biggest missing piece blocking everything else in `RHI/`.
- **Restore `ToNVRHI(Zephyr::GraphicsAPI) -> nvrhi::GraphicsAPI`** in `RHI/GraphicsAPI.h`. It
  existed in the old `Renderer/GraphicsAPI.h` and was silently dropped in the RHI migration;
  `ShaderFactory` and device creation both need this conversion.
  See `docs/rhi.md` Design Notes.
- **`RHI/CommadBuffer.h` is renamed to `CommandBuffer.h`** and filled in as `RHI::CommandList`,
  a thin wrapper over `nvrhi::ICommandList` (open/close/submit). It is currently an empty,
  untyped placeholder with a typo'd filename and is not referenced anywhere, so renaming it is
  zero-risk. Nothing above nvrhi can record or submit work without it.
- **Vulkan ships first.** It already has the most scaffolding (`VulkanDevice` stub, full
  `VulkanDeviceManager` reference implementation) and gives the fastest path to a visible,
  verifiable checkpoint (a cleared/triangle-rendering window) that the shader pipeline work can
  then build on. D3D11 follows the same port pattern from `D3D11DeviceManager`. D3D12 comes
  last because it has no in-tree reference and is the largest single chunk of new code.
  Vertical slices, in order: Vulkan device+surface+present → slang shader pipeline (verified on
  Vulkan) → D3D11 device+surface+present → D3D12 device+surface+present → delete the orphaned
  stack.
- **Verification is a rendered frame, not just "compiles."** Every backend task's acceptance
  criteria requires the Editor to actually open a window and present a frame on that backend
  (`GraphicsAPI` value swapped in `Application.cpp`), because `RHI::Device::Create()` "compiling
  and constructing" is exactly the trap the current skeleton is already in (see `docs/rhi.md`:
  `VulkanDevice` "compiles and can be constructed... but does not actually create a Vulkan
  instance").
- **`IRenderPass`/`ImGuiRenderPass` are deleted outright, then rebuilt against the new API.**
  They only exist to plug into `DeviceManager`'s render-pass list; a repo-wide grep (recorded in
  `docs/renderer.md`) confirms zero references outside `Renderer/` itself, so deleting them in
  Phase 5 loses no live functionality. ImGui rendering is then re-added in Phase 6 as new code
  hooked to `RHI::Device`/`RHI::CommandList`/`RHI::Surface` instead of `DeviceManager`/
  `IRenderPass` — the deleted files' logic (font atlas upload, vertex/index buffer growth,
  per-texture binding-set cache, PSO) is recovered from git history as a *reference*, not
  restored as a compiling dependency. This is deliberately sequenced after the legacy-stack
  deletion (Phase 5) rather than ported in place, so the new ImGui renderer is written against
  the finished `RHI` surface, not against whatever `RHI` looked like mid-port.

## Task List

### Phase 0: Foundation fixes (unblocks everything else)
- [x] Task 1: Fix `Build.lua` NVRHI directory casing (`Engine/Vendor/NVRHI` → `Engine/Vendor/nvrhi`)
- [x] Task 2: Restore `ToNVRHI()`, fix `Device::Create` fallthrough bug, add `Device::GetNvrhiDevice()`
- [ ] Task 3: Rename `CommadBuffer.h` → `CommandBuffer.h`, implement `RHI::CommandList`

### Checkpoint: Foundation
- [ ] `Engine`/`Editor` build clean on `Build.lua`'s current state (Vulkan-only, still non-functional) with no new warnings from Task 1–3 changes
- [ ] Confirm `Device::Create(GraphicsAPI::D3D11)` / `Create(D3D12)` no longer silently fall off the end (still fine to return `nullptr` until Phase 3/4 land, but must not be UB)

### Phase 1: Vulkan vertical slice
- [ ] Task 4: Port Vulkan instance/physical-device/logical-device creation into `RHI::VulkanDevice`
- [ ] Task 5: Implement `RHI::VulkanSurface` (window surface, swapchain, resize, acquire/present)
- [ ] Task 6: Wire `Renderer` to create a `Surface` from `Window`, record a clear-color command list per frame, present

### Checkpoint: Vulkan renders
- [ ] Editor opens a window and shows a solid clear color via Vulkan
- [ ] Resize and close do not crash
- [ ] Human review before starting the shader pipeline

### Phase 2: Slang → NVRHI shader pipeline
- [ ] Task 7: Adapt `ShaderFactory` construction to a `Ref<RHI::Device>`-based entry point
- [ ] Task 8: Add a minimal vertex+pixel `.slang` shader pair and PSO creation in `Renderer`, render a triangle

### Checkpoint: Shader pipeline proven
- [ ] Editor renders a colored triangle on Vulkan via a `.slang` source compiled through `ShaderFactory` to SPIR-V
- [ ] `.bin` cache file is written next to the `.slang` source and reused on second run
- [ ] Human review before starting D3D11/D3D12

### Phase 3: D3D11 vertical slice
- [ ] Task 9: Port D3D11 device/adapter creation into `RHI::D3D11Device`
- [ ] Task 10: Implement `RHI::D3D11Surface` (DXGI swapchain, render target, resize, present)
- [ ] Task 11: Verify the same triangle renders via D3D11 (DXBC path)

### Checkpoint: D3D11 renders
- [ ] Switching `GraphicsAPI::VULKAN` → `GraphicsAPI::D3D11` in `Application.cpp` is the only code change needed to render the same triangle
- [ ] Resize and close do not crash

### Phase 4: D3D12 vertical slice (new code, no in-tree reference)
- [ ] Task 12: Implement `RHI::D3D12Device` (DXGI factory/adapter, `ID3D12Device`, command queue, `nvrhi::d3d12::createDevice`)
- [ ] Task 13: Implement `RHI::D3D12Surface` (flip-model swapchain, per-frame fence sync, present)
- [ ] Task 14: Verify the same triangle renders via D3D12 (DXIL path)

### Checkpoint: All three backends render
- [ ] Vulkan, D3D11, D3D12 all render the identical triangle by only changing the `GraphicsAPI` value
- [ ] Human review before deleting the legacy stack

### Phase 5: Delete the orphaned legacy stack
- [ ] Task 15: Delete `DeviceManager`, `D3D11DeviceManager`, `VulkanDeviceManager`, `IRenderPass`, `ImGuiRenderPass`; remove dead includes; update `docs/rhi.md`/`docs/renderer.md`

### Checkpoint: Legacy stack removed
- [ ] Zero references to `DeviceManager`/`D3D11DeviceManager`/`VulkanDeviceManager` remain in the tree
- [ ] Full solution builds clean on all three `GraphicsAPI` values
- [ ] `docs/` reflects the new, single-device-stack reality

### Phase 6: Re-add ImGui rendering against the new RHI
- [ ] Task 16: Port ImGui draw-data rendering (font atlas, buffers, PSO, binding-set cache) onto `RHI::Device`/`RHI::CommandList`
- [ ] Task 17: Wire GLFW input into ImGui and enable `Application::OnImGui`

### Checkpoint: Complete
- [ ] Editor panels (Hierarchy, Info, etc.) render and accept mouse/keyboard input on all three `GraphicsAPI` backends
- [ ] `docs/` reflects the new, single-device-stack reality including the re-added ImGui path

## Risks and Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| D3D12 has zero in-tree or vendored reference implementation | High — largest unknown-unknown in the plan | Scheduled last, after the Vulkan/D3D11 port pattern is proven; budget it as 3 tasks instead of the 3 used for D3D11/Vulkan; treat `D3D11Device`'s structure (not its D3D11-specific calls) as the template for command-queue/fence/present shape |
| `ShaderFactory`'s slang session/target selection (`m_Device->getGraphicsAPI()`) needs `nvrhi::GraphicsAPI`, which only exists once `Device::GetNvrhiDevice()` exists | Medium — blocks Phase 2 | Sequenced explicitly after Task 2 |
| Deleting `IRenderPass`/`ImGuiRenderPass` (Phase 5) removes the only ImGui-via-nvrhi renderer in the tree before its replacement exists (Phase 6) | Medium — Editor UI renders nothing between Phase 5 and Phase 6 checkpoints | Accepted as a temporary state within this plan's execution, not as a final deliverable gap; Phase 6 is mandatory, not a follow-up. Recover the deleted logic via `git show <pre-deletion-commit>:<path>` when writing Task 16/17, don't rewrite from memory |
| Vulkan validation layer / device extension list drift between `VulkanDeviceManager` (donut-derived) and a fresh `RHI::VulkanDevice` | Low-medium — subtle runtime validation errors | Port the extension lists and queue-family logic verbatim first; don't "clean up" simultaneously with porting |
| `Build.lua` NVRHI casing fix could interact with an already-generated `Zephyr.sln`/`.vcxproj` in the tree | Low | Re-run `Scripts/Setup-Windows.bat` after Task 1, don't hand-edit the generated project files |

## Open Questions

- Should `GraphicsAPI` selection become a runtime/CVar option, or is swapping the hardcoded value
  in `Application.cpp` (as used for verification throughout this plan) an acceptable permanent
  state? Not specified by the request; left as-is (hardcoded, swappable) unless directed otherwise.
- D3D12 ray tracing / `rtxmu` integration (already bundled into the vendored nvrhi build) is not
  requested and is explicitly out of scope — `D3D12Device` targets baseline rasterization only.
