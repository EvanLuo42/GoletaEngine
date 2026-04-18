# GoletaEngine — Conventions for Claude

## Naming (LLVM style)

Follow the [LLVM Coding Standards](https://llvm.org/docs/CodingStandards.html#name-types-functions-variables-and-enumerators-properly) for C++:

- **Types** (class / struct / enum / using-alias): `UpperCamelCase`
  - `Vec4`, `SimdConfig`, `RenderPass`
- **Functions / methods**: `lowerCamelCase`
  - `normalize()`, `fromAxisAngle()`, `transformPoint()`
- **Variables** (locals, parameters, members, globals): `UpperCamelCase`
  - `Vec4f Data;`, `int RowCount;`, `float Radians`
- **Enumerators**: `UpperCamelCase`
  - `enum class Format { Rgba8Unorm, Rgba16Float };`
- **Namespaces**: `lowercase`
  - `goleta`, `goleta::math`, `goleta::simd`
- **Macros**: `UPPER_SNAKE_CASE` with `GOLETA_` prefix
  - `GOLETA_FORCEINLINE`, `GOLETA_SIMD_AVX2`, `GOLETA_ALIGN16`
- **Files**: `UpperCamelCase.h` / `.cpp` / `.inl`
  - `SimdOps.h`, `Impl_SSE.inl`

Two-letter acronyms stay uppercase (`IOContext`); three-or-more go title-case (`HttpClient`, `SimdOps`, not `SIMDOps`).

## Comments

Default to writing **no** comments. The bar is: a reader who already knows C++ and can see the identifier gets nothing from the comment — delete it.

**Write:**
- Doxygen `///` or `/** */` on public class / struct / enum / function / method declarations (`@brief`, `@param`, `@return`, `@tparam` as needed).
- A Doxygen `@file` block at the top of each header / source file, with a one-line `@brief`. Do not use LLVM-style `===-- --===` prose banners; if there's nothing worth saying beyond the filename, omit the block entirely.
- Short inline notes *only when the WHY is non-obvious*: a hidden constraint, a subtle invariant, a platform-specific workaround, surprising numerical behavior, ABI pitfall.

**Do not write:**
- Comments that restate what the code does (`// increment i`).
- Comments that name the caller, ticket, or task (`// added for the lighting refactor`, `// fixes #123`) — that belongs in git history / PR description.
- Section dividers, anywhere — inside function bodies *or* between declarations (`// ---- Vec4 ----`, `// ---- setup ----`). Let the identifiers and whitespace do the grouping.
- Placeholder or changelog comments (`// TODO: will be removed`, `// old code below`) left in committed code.
- Multi-paragraph docstrings. Keep Doxygen tight; one sentence of `@brief` plus tags is usually enough.
- Redundant `@return void` or `@param X the X`.

If a comment survives review, it should be because removing it would genuinely confuse the next reader.

## Module directory layout

Each engine module has up to three top-level directories:

- **`Public/`** — exported API. Headers here define the module's contract with downstream code.
- **`Internal/`** — also on the public include path (so inline chains in `Public/` headers can resolve), but documented as implementation detail. Downstream code should not include `Internal/` headers directly, and they are **not** installed with the module.
- **`Private/`** — fully internal sources; not exposed to any other module.

## Public API documentation (mandatory)

Every public class / struct / enum / function / method declared in a `Public/` header must carry a Doxygen `@brief`. Naked declarations in a public header are a review miss.

- Add `@param` only when the parameter's role isn't obvious from its name or type.
- Add `@return` only when the return value carries extra semantics (units, invariants, edge cases).
- Add `@note` for constraints or non-obvious invariants (e.g. "expects unit quaternion", "only exact with uniform scale", "undefined for zero-length input").
- Keep it tight. One sentence `@brief` plus the minimum tags needed.

**`Internal/` code is exempt** from the mandatory `@brief` requirement. Document it when the reasoning is non-obvious, but don't block on per-symbol coverage -- it's not part of the module contract.

## Unit tests (mandatory)

Every public function / method declared in a `Public/` header should have at least one unit test under the owning module's `Tests/` directory, exercising both the happy path and at least one edge case or non-trivial branch.

If a function genuinely can't be unit tested, mark it with a `@note` explaining why and skip the test. Valid reasons:

- Depends on hardware not available on the host (real GPU, specific ISA branch).
- Opens a window, hits the network, spawns processes, or needs a live runtime environment.
- A template with a specialization only reachable at link time against another module.

"It's trivial" is not a valid reason to skip -- a one-line getter is still testable in one line.

**`Internal/` code is exempt** from the per-function unit test mandate. Direct tests are still welcome when the op has subtle semantics (e.g. SIMD shuffle patterns), but transitive coverage via `Public/` tests is acceptable.
