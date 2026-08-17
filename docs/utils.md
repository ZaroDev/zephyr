# Utils

`Utils` is a small grab-bag of standalone helper functions that don't belong to any specific
engine subsystem: a byte-to-megabyte conversion helper and a header-only string/hashing toolkit
(prefix/suffix checks and a compile-time FNV-1a string hash used as a fast, comparable key for
named lookups like CVars). It exists as a landing spot for small, dependency-light utilities
that multiple unrelated submodules need.

## Key Files

| File | Purpose |
|---|---|
| `Utils/DataConversion.h` / `.cpp` | `BytesToMB(u64) -> u32` — a single unit-conversion helper. |
| `Utils/StringUtils.h` | Header-only: `fnv1a_32`, `const_strlen`, `StartsWith`, `EndsWith`, and the `StringHash` wrapper type. |

## Key Types & APIs

### `DataConversion.h/.cpp`
- `BytesToMB(u64 bytes) -> u32` — integer division `bytes / 1024 / 1024`, truncating both the
  precision loss from `u64 → u32` (via `static_cast<u32>(bytes)` applied *before* the division)
  and any fractional megabyte. See Design Notes for the ordering issue this creates.

### `StringUtils.h` (namespace `Zephyr::StringUtils`)
All `constexpr`/`inline`, header-only:
- `fnv1a_32(const char* s, SizeT count) -> u32` — a recursive `constexpr` FNV-1a 32-bit hash.
  Recursion walks from `s[count]` down to `s[0]`, so it can be evaluated entirely at compile
  time for string literals.
- `const_strlen(const char* s) -> SizeT` — a `constexpr` strlen replacement (needed because
  `std::strlen` isn't guaranteed `constexpr` in this codebase's toolchain/standard usage).
- `StartsWith(StrView value, StrView beginning) -> bool` / `EndsWith(StrView value, StrView
  ending) -> bool` — straightforward size-then-`std::equal` checks; `EndsWith` compares via
  reverse iterators.
- `StringHash` — a small wrapper struct around a computed `u32` (via `fnv1a_32`), constructible
  from a `const char*`, a `(const char*, SizeT)` pair, or a `StrView`, all `constexpr`. It has
  an implicit `operator u32()` so a `StringHash` can be used directly as an unordered-map key or
  compared against a raw `u32`. This is the type `CVarManager`'s API (`GetCVar`, `GetBoolCVar`,
  etc. — see `docs/core.md`) uses to identify CVars by name without storing/comparing full
  strings at lookup time.

## Design Notes

- **`BytesToMB` truncates the `u64 → u32` cast before dividing, not after**
  (`static_cast<u32>(bytes) / 1024u / 1024`). For any `bytes` value at or above 4 GiB
  (`UINT32_MAX + 1`), the initial cast wraps/truncates before the division ever happens, giving
  a wrong (wrapped-around) megabyte count instead of a merely-out-of-range one. For a memory or
  asset-size reporting helper — where multi-gigabyte values are plausible — this is a
  correctness bug, not just a design choice.
- **`BytesToMB` currently has no call sites anywhere in the Engine or Editor** as of this pass —
  it's defined and exported but unused, so its bug hasn't surfaced in practice yet.
- **`fnv1a_32`'s recursion depth equals the string length.** For very long strings this could
  exhaust `constexpr` evaluation limits or (if not evaluated at compile time) create real
  recursive call overhead at runtime — fine for the short, hand-written identifiers (CVar
  names, etc.) this is actually used for, but not a general-purpose runtime string hasher.
- **`StringHash::operator u32()` is not `const`-qualified**, meaning a `const StringHash&`
  cannot be implicitly converted to `u32` — every current use in `CVarManager.h` takes
  `StringHash` by value, which sidesteps this, but it would surprise a caller who tried to hold
  one by `const&`.
- **No case-insensitive or Unicode-aware variants** of `StartsWith`/`EndsWith` — both are
  byte-wise `std::equal` comparisons over whatever encoding the `StrView` happens to hold.

## Dependencies

- **Depends on:** `Zephyr::Core::Base.h` (`StrView`, `SizeT`-adjacent aliases) and
  `Zephyr::Math::MathTypes.h` (pulled in by `StringUtils.h`, though not obviously used by
  anything in that header beyond what `Base.h`/`BasicTypes.h` already provide — likely picked
  up for the `u32`/`SizeT` aliases that also flow through `MathTypes.h`'s own include of
  `BasicTypes.h`). `DataConversion.h/.cpp` only needs `BasicTypes.h`'s `u32`/`u64`.
- **Depended on by:** `Zephyr::Core::CVarManager` (`StringHash` is the CVar lookup key type —
  `CVarManager.h` includes `Utils/StringUtils.h` directly), and `Zephyr::FileSystem`
  (`FileSystem.cpp` uses `StringUtils::EndsWith` to check for a trailing path separator when
  normalizing paths). `DataConversion.h`'s `BytesToMB` has no current consumers.
