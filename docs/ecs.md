# ECS

The `ECS` submodule is Zephyr's scene/entity representation: a thin, `entt`-backed
Entity-Component-System that owns the set of game objects in a loaded scene, their component
data, and the operations (create/destroy/duplicate/lookup) needed to manage them. It exists so
that gameplay/editor/renderer code has one shared, stable way to represent "a thing in the
world" instead of ad hoc object graphs — the `Renderer`, the Editor's `HierarchyPanel`/
`ScenePanel`, and `Application::GetActiveScene()` all key off this representation.

## Key Files

| File | Purpose |
|---|---|
| `ECS/Scene.h` / `.cpp` | `Scene`: owns the `entt::registry` and a `UUID → entt::entity` lookup map; entity lifecycle (create/destroy/duplicate/find). |
| `ECS/Entity.h` / `.cpp` | `Entity`: a lightweight handle (`entt::entity` + owning `Scene*`) with templated component accessors. |
| `ECS/Components.h` | The concrete component types (`IDComponent`, `TagComponent`, `MeshComponent`, `TransformComponent`, `LightComponent`) and the `ComponentGroup`/`AllComponents` machinery used for bulk-copy operations. |

## Key Types & APIs

### `Scene` (`Scene.h/.cpp`)
Wraps an `entt::registry` (`m_Registry`) plus an `std::unordered_map<UUID, entt::entity>`
(`m_Entities`) that lets code resolve a stable, serializable `UUID` back to the live (and
otherwise-opaque, session-local) `entt::entity` handle.

- `CreateEntity(name)` creates a raw `entt` entity, wraps it in an `Entity`, and unconditionally
  attaches `IDComponent` (fresh random `UUID`), `TransformComponent` (identity), and
  `TagComponent` (given name, or `"Entity"` if empty) — every entity in a `Scene` is guaranteed
  to have these three components. It also registers the entity in `m_Entities` keyed by its new
  UUID.
- `DestroyEntity(entity)` removes the UUID mapping and destroys the `entt` entity. It does not
  recurse into any parent/child relationship — there is no hierarchy component in this
  submodule as it stands (see Design Notes).
- `DuplicateEntity(entity)` creates a new entity with the same name, then copies over
  `TransformComponent`, `MeshComponent`, and `LightComponent` from the source if present, via
  `CopyComponentIfExists<AllComponents>` (`AllComponents` is defined in `Components.h`).
  `IDComponent` is deliberately *not* copied (the new entity gets its own fresh UUID from
  `CreateEntity`), which is correct — UUIDs must stay unique per entity.
- `FindEntityByName(name)` does a linear scan over all `TagComponent`s; `GetEntityByUUID(uuid)`
  is the O(1) map lookup. Both return a default-constructed (null) `Entity` on miss.
- `GetAllEntitiesWith<Components...>()` is a thin forward to `entt::registry::view<...>()`,
  used by systems (e.g. the renderer) to iterate all entities that have a given component set.
- A free function template `CopyComponent<Component...>(dst, src, enttMap)` (in `Scene.cpp`,
  file-local) supports the same bulk-copy pattern at the *registry* level (as opposed to
  `CopyComponentIfExists`, which works at the *entity* level) — it is defined but has no call
  site in this file; it looks intended for a future whole-scene copy/clone operation that
  doesn't exist yet.

### `Entity` (`Entity.h/.cpp`)
A cheap-to-copy handle: `entt::entity m_EntityHandle` + `Scene* m_Scene` (non-owning). All
component operations are templated members that forward straight to the owning `Scene`'s
registry:

- `AddComponent<T>(args...)` — asserts the entity does *not* already have `T`, then
  `emplace`s it.
- `AddOrReplaceComponent<T>(args...)` — `emplace_or_replace`, used by `DuplicateEntity`'s
  copy path since the destination may already have a default-constructed component (e.g.
  `TransformComponent`, added unconditionally by `CreateEntity`).
- `GetComponent<T>()` — asserts `HasComponent<T>()`, then returns the component by reference.
- `HasComponent<T>()` / `RemoveComponent<T>()` — presence check / removal (removal also
  asserts presence first).
- `GetUUID()` / `GetName()` are convenience shortcuts to `GetComponent<IDComponent>().ID` and
  `GetComponent<TagComponent>().Tag` — they rely on every entity always having those two
  components, which `Scene::CreateEntity` guarantees but nothing in `Entity` itself enforces
  (e.g. a manually-constructed `Entity{handle, scene}` around an entity that skipped
  `CreateEntity` would assert/crash on `GetUUID()`).
- Conversion operators to `bool` (null check against `entt::null`), `entt::entity`, and `u32`
  make `Entity` interoperate directly with `entt` APIs and integer-keyed UI code (e.g. ImGui
  tree node ids).

### Components (`Components.h`)
Plain data structs, all default-constructible and copyable, deliberately kept free of behavior:

- `IDComponent { UUID ID; }` — identity; always present.
- `TagComponent { std::string Tag; }` — display name; always present.
- `TransformComponent { V3 Translation; Quaternion Rotation; V3 Scale; }` — always present;
  provides `GetTransform()` (composes to a `Mat4` via `Math::CreateTransform`) and a
  `Mat4`-taking constructor that decomposes a matrix back into T/R/S via
  `Math::DecomposeTransform` (see `docs/math.md`).
- `MeshComponent { u32 MeshId; u32 MaterialId; }` — indices into whatever mesh/material
  registry the renderer owns; this component carries no ownership or validation of those ids.
- `LightComponent` — a tagged union-by-convention (`Type LightType` plus `Color`/`Direction`/
  `Radius` fields that are only meaningful for some light types); there are two constructors,
  one implicitly for point-like lights (color + radius) and one for directional/spot-like
  lights (color + direction), but nothing in the type prevents constructing an inconsistent
  combination (e.g. a `DIRECTIONAL` light with a `Radius` left uninitialized — the
  radius-taking constructor does not set `Direction` from a meaningful default other than
  `V3(0.0)` cast from a scalar).
- `ComponentGroup<Component...>` / `AllComponents` — a compile-time list
  (`TransformComponent, MeshComponent, LightComponent`) used purely as a tag type so
  `CopyComponent`/`CopyComponentIfExists` can be invoked once with "all the copyable
  components" instead of being called out per type. `IDComponent` and `TagComponent` are
  intentionally excluded (handled specially by `Scene::CreateEntity`/`DuplicateEntity`).

## Design Notes

- **No hierarchy/parenting.** There is no `ParentComponent`/`ChildrenComponent` or equivalent
  in this submodule. `DestroyEntity` is a flat, single-entity operation. Any parent-child scene
  graph must live elsewhere (or doesn't exist yet).
- **`AllComponents` will silently miss new component types.** Anyone adding a new component
  struct to `Components.h` that should participate in duplication must remember to also add it
  to the `AllComponents` alias — nothing enforces this, so it's an easy place for
  duplicate/copy to quietly drop data for a newly-added component.
- **`Entity`'s invariants are load-bearing but unchecked at the type level.** `GetUUID()`/
  `GetName()` assume `IDComponent`/`TagComponent` are always present. This holds for anything
  created via `Scene::CreateEntity`, but a default-constructed `Entity{}` (null handle, null
  scene) or a hand-built `Entity{handle, scene}` around an entity created directly through
  `entt::registry` (bypassing `Scene::CreateEntity`) would violate it.
- **`Scene` is declared copyable/movable** (`DEFAULT_MOVE_AND_COPY(Scene)` in `Scene.h`), which
  for an `entt::registry`-holding type is a real (if expensive) deep copy of the registry —
  worth being deliberate about at call sites, since copying a `Scene` is not a cheap operation
  and every live `Entity` handle referencing the *old* `Scene*` becomes invalid/dangling
  relative to the copy (handles store a raw `Scene*`, not a reference-counted owner).
- **`HierarchyPanel` in the Editor is a `friend class` of `Scene`,** meaning the Editor UI
  reaches directly into `Scene`'s private registry rather than going through `Scene`'s public
  API — an intentional but tight coupling between ECS internals and one specific editor panel.
- **File-local `CopyComponent` (registry-level, in `Scene.cpp`) is currently dead code** — it
  compiles and is used by nothing in this file; only the entity-level `CopyComponentIfExists`
  is actually called (from `DuplicateEntity`). It reads as scaffolding for a future
  "duplicate/merge whole scene" feature.

## Dependencies

- **Depends on:** `entt` (vendored, `Entt/entt.hpp`) for the registry/view machinery;
  `Zephyr::Core` for `UUID` (entity identity) and `Assert.h` (`CORE_ASSERT` in `Entity`'s
  component accessors); `Zephyr::Math` (`MathTypes.h`, `Math.h`) for `V3`/`Quaternion`/`Mat4`
  and the `CreateTransform`/`DecomposeTransform` helpers used by `TransformComponent`.
- **Depended on by:** `Application` (`GetActiveScene()` returns `Ref<ECS::Scene>` — see
  `docs/core.md`), the Editor (`HierarchyPanel`, `ScenePanel`, `App.cpp`/`App.h` all reference
  `ECS::Scene`/`ECS::Entity`), and the Renderer submodule, which is expected to iterate
  `Scene::GetAllEntitiesWith<...>()` to find drawable/lit entities (its `Camera.cpp` already
  pulls in `Math` types shared with `Components.h`).
