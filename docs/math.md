# Math

The `Math` submodule defines Zephyr's vector/matrix/quaternion/color vocabulary (thin aliases
over `glm`) and a handful of transform-composition helpers built on top of them. It exists so
the rest of the engine — ECS components, the renderer, the camera — shares one set of math
types instead of every submodule picking its own, and so `glm`'s configuration (coordinate
convention, depth range, included headers) is decided in exactly one place.

## Key Files

| File | Purpose |
|---|---|
| `Math/MathTypes.h` | Type aliases over `glm` (`V2`/`V3`/`V4`, integer/unsigned variants, `Mat3`/`Mat4`, `Quaternion`, `Color`), plus `glm` configuration defines and math constants. |
| `Math/Math.h` / `.cpp` | Free functions: `CreateTransform`, `DecomposeTransform`, `Lerp`. |

## Key Types & APIs

### `MathTypes.h`
Pure aliasing header, no logic:
- `V2/Iv2`, `V3/Iv3`, `V4/Iv4` — float and signed-32-int vector variants; `U32V2/3/4` and
  `S32V2/3/4` add explicit unsigned/signed-32 variants on top.
- `Mat3` (`glm::mat3x3`), `Mat4` (`glm::mat4x4`, documented as assuming right-handed
  coordinates), `Quaternion` (`glm::quat`).
- `Color` (= `glm::vec4`), `Color3` (= `V3`), `Color4` (= `V4`) — `Color3`/`Color4` are plain
  aliases of the vector types, not distinct types, so nothing stops passing a `V3` position
  where a `Color3` is expected or vice versa.
- `Math::c_Pi` and `Math::c_Epsilon` — the two math constants defined in this submodule.
- On `PLATFORM_WINDOWS` (the only supported platform — see `docs/core.md`), it forces
  `GLM_FORCE_DEPTH_ZERO_TO_ONE` (0-to-1 NDC depth, matching D3D/Vulkan rather than OpenGL's
  -1-to-1) and `GLM_FORCE_XYZW_ONLY` (drops `glm`'s r/g/b/a and s/t/p/q swizzle unions,
  slightly reducing union-based UB surface). `GLM_ENABLE_EXPERIMENTAL` is also defined here to
  unlock `glm/gtx/quaternion.hpp`.

### `Math.h` / `Math.cpp`
- `CreateTransform(translation, rotation, scale) -> Mat4` — composes T * R * S (translation
  applied last conceptually, i.e. `glm::translate(...) * glm::toMat4(rotation) *
  glm::scale(...)`), the standard TRS order for a local-to-parent/world matrix.
- `DecomposeTransform(matrix, out translation, out rotation, out scale)` — the inverse
  operation via `glm::decompose`, which also produces (and discards) a skew vector and a
  perspective `V4` that this function doesn't expose. This is what
  `ECS::TransformComponent`'s `Mat4`-taking constructor uses to rebuild T/R/S from an arbitrary
  matrix (see `docs/ecs.md`).
- `Lerp(a, b, f) -> float` — plain scalar linear interpolation (`a + f * (b - a)`); no
  clamping of `f`, so values outside `[0, 1]` extrapolate rather than clamp.

## Design Notes

- **This is a thin wrapper, not a math library.** Almost everything here is a `glm` alias or a
  one-line composition of `glm` calls; the intent is clearly "give the engine its own type
  names and one configuration point" rather than to reimplement vector math.
- **`DecomposeTransform` silently discards skew and perspective.** `glm::decompose` computes
  those too, but `Math::DecomposeTransform`'s signature has no output for them — any matrix
  with meaningful skew or a non-identity perspective row will round-trip lossily through
  `CreateTransform(DecomposeTransform(m))`.
- **`Lerp` is untyped/unclamped and float-only.** There's no vector overload (`Lerp(V3, V3,
  float)`) in this submodule — callers currently must do component-wise `Math::Lerp` calls or
  use `glm::mix` directly if they need to interpolate a vector.
- **Right-handedness is documented only in a comment** (`Mat4` alias comment: "assumes
  right-handed coordinates") — `glm`'s default is already right-handed, so this isn't enforced
  by any code, just an expectation callers should be aware of.

## Dependencies

- **Depends on:** `glm` (vendored) — `glm/glm.hpp`, `glm/gtx/quaternion.hpp`,
  `glm/gtc/quaternion.hpp` from `MathTypes.h`; `glm/gtc/matrix_transform.hpp` and
  `glm/gtx/matrix_decompose.hpp` from `Math.cpp`. Also `Zephyr::Core` for `BasicTypes.h`
  (`f32`) and `PlatformDetection.h` (the `PLATFORM_WINDOWS` gate on the `glm` config defines).
- **Depended on by:** `ECS::Components` (`TransformComponent`, `LightComponent` use
  `V3`/`Quaternion`/`Mat4` and call `Math::CreateTransform`/`DecomposeTransform` directly —
  see `docs/ecs.md`), `Core::UUID.h` (includes `MathTypes.h` for `u64`... actually for the
  basic-type dependency chain it pulls in), `Renderer/Camera.h`/`.cpp`, and the Editor's
  `HierarchyPanel.cpp` (editing transform fields in the UI). `Log.h` in `Core` also depends on
  `MathTypes.h` indirectly to provide the `operator<<` overloads for `glm` vector/matrix/quat
  types used by the logging macros.
