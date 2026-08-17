# Time

The `Time` submodule tracks per-frame timing for the engine's main loop: delta time, elapsed
time since start, and (partially) FPS/average-frame-time statistics. It exists so that
`Application::Run()` and any per-frame system have one shared, global notion of "how much time
passed this frame" instead of each system running its own clock.

## Key Files

| File | Purpose |
|---|---|
| `Time/Time.h` | Public API: `GetDeltaTime`, `GetFPS`, `GetTimeSinceStart`, `GetAverageFrameTime`, `GetAverageFPS`, `StartTimeUpdate`, `EndTimeUpdate`. |
| `Time/Time.cpp` | Implementation: file-local (anonymous-namespace) state updated by `StartTimeUpdate`/`EndTimeUpdate`. |

## Key Types & APIs

`Time` is a free-function API over hidden global state (an anonymous namespace in `Time.cpp`
holding `g_DeltaTime`, `g_LastTime`, `g_TimeSinceStart`, `g_FrameCount`) — there is no `Time`
class/instance; it behaves as a process-wide singleton.

- `StartTimeUpdate()` — call at the top of a frame; records `g_LastTime =
  std::chrono::high_resolution_clock::now()`.
- `EndTimeUpdate()` — call at the end of a frame; computes the elapsed duration since
  `StartTimeUpdate()`, stores it (×1000, see Design Notes) into `g_DeltaTime`, accumulates
  `g_TimeSinceStart`, and increments `g_FrameCount`.
- `GetDeltaTime()` — returns `g_DeltaTime` as last computed by `EndTimeUpdate()`.
- `GetFPS()` — returns `1000.0f / g_DeltaTime`.
- `GetTimeSinceStart()` — returns accumulated seconds (see Design Notes on the unit
  inconsistency) since the process/loop started calling `EndTimeUpdate()`.
- `GetAverageFrameTime()` / `GetAverageFPS()` — declared in the header but **stubbed** in the
  implementation, always returning `0.f` / `0`. `g_FrameCount` is incremented every frame but
  never read anywhere, so it exists only to eventually back these two functions.

Usage pattern, as seen in `Application::Run()` (`docs/core.md`):
```cpp
Time::StartTimeUpdate();
float deltaTime = Time::GetDeltaTime(); // note: reads *last* frame's value, before EndTimeUpdate runs
// ... update/render the frame using deltaTime ...
Time::EndTimeUpdate();
```

## Design Notes

- **`GetDeltaTime()` is measured in milliseconds, not seconds, despite the name reading like a
  standard "delta time in seconds" API.** `EndTimeUpdate()` does
  `g_DeltaTime = duration.count() * 1000.0f;` where `duration` is a
  `std::chrono::duration<float>` (seconds by default) — so `g_DeltaTime` ends up in
  milliseconds. `GetFPS()`'s `1000.0f / g_DeltaTime` is internally consistent with that (it
  would be wrong if `g_DeltaTime` were actually seconds), but every consumer of
  `Time::GetDeltaTime()` outside this file needs to know it's getting milliseconds, not the
  seconds most engine code conventionally expects. This is the single most important thing to
  know about this submodule, since `Application::Run()` forwards this value unmodified as the
  `deltaTime` parameter to every `On*Update`/`On*Render` virtual.
- **`GetTimeSinceStart()`, unlike `GetDeltaTime()`, is accumulated in seconds**
  (`g_TimeSinceStart += duration.count();` — no ×1000). So within this same submodule,
  `GetDeltaTime()` and `GetTimeSinceStart()` use different units, which is easy to trip over.
- **`StartTimeUpdate()`/`EndTimeUpdate()` must be called in that order, once per frame, from
  the same thread** — there is no synchronization on the global state, and `GetDeltaTime()`
  called between `StartTimeUpdate()` and `EndTimeUpdate()` returns the *previous* frame's delta
  (this is in fact how `Application::Run()` uses it — it reads `GetDeltaTime()` right after
  `StartTimeUpdate()`, before that frame's `EndTimeUpdate()` has run, so `deltaTime` for frame N
  is actually the duration of frame N-1).
- **No fixed-timestep or clamping support.** There's no accumulator, max-delta clamp, or
  time-scale/pause multiplier — a stalled frame (e.g. a breakpoint or GPU hang) will produce an
  arbitrarily large `g_DeltaTime` with nothing downstream to clamp it, which could cause large
  simulation steps if physics/gameplay code ever consumes this value directly (relevant since
  `Core`'s physics pipeline is currently unreachable anyway — see `docs/core.md`).
- **Not thread-safe.** All state is plain (non-atomic) global variables in an anonymous
  namespace; this API assumes it's driven from a single main-loop thread.

## Dependencies

- **Depends on:** `<chrono>` (via `pch.h`) for `std::chrono::high_resolution_clock`/
  `steady_clock`; `Zephyr::Core::BasicTypes` for `u64` (`g_FrameCount`). No dependency on any
  other Engine submodule beyond `Core`.
- **Depended on by:** `Application::Run()` (`Core/Application.cpp`) drives the per-frame
  `StartTimeUpdate`/`GetDeltaTime`/`EndTimeUpdate` sequence; the Editor's `InfoPanel.cpp`
  reads `Time` values directly (e.g. to display FPS/frame time in a debug UI panel).
