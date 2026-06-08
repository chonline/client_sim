# M1 Acceptance Report

> Date: 2026-06-08
> Milestone: M1 (基础设施 / Infrastructure)
> Spec: `docs/superpowers/specs/2026-06-08-m1-finish-design.md`
> Plan: `docs/superpowers/plans/2026-06-08-m1-finish.md`

## Summary

M1 was originally scoped to deliver a `wzlib` resource parser plus a
minimal Qt UI shell. The first M1 commit (`30365e3`) shipped the
`wzlib` library, four unit-test executables, and the `wzdump` CLI
tool. This milestone-closing change set delivers the rest of M1:

- A Qt 6.5+ desktop application `client_sim` that opens a directory
  of `.wz` files, browses the resulting virtual tree, and shows
  `WzPng` payloads in a zoomable image viewer.
- The new test target `test_wz_structure` with five synthetic
  wzlib-invariant tests (decodeRgba byte order, in-memory node
  resolution) and stubbed real-WZ tests that are documented but not
  yet added (the project uses a custom `wztest` framework rather
  than GoogleTest, so the plan's `GTEST_SKIP` references are
  replaced with file-level docs).
- Top-level CMake gating so that wzlib + tests + tools still
  build cleanly on machines without Qt6.

## What was verified locally (no Qt required)

The dev environment has g++/cmake/ninja, no Qt6, and no network
access to install Qt6. The following were verified on the dev box:

| Check | Command | Result |
| --- | --- | --- |
| wzlib + tests + tools build cleanly without Qt6 | `cmake --build build` | PASS |
| All 5 test executables pass | `ctest --test-dir build --output-on-failure` | PASS (5/5) |
| `app/` sub-project is correctly skipped when Qt6 is absent | Inspect CMake summary line | PASS (`Qt6 (Widgets): FALSE`) |
| The new `test_wz_structure` target runs its synthetic tests | `ctest -R test_wz_structure` | PASS (5/5: 2 WzPng, 3 WzNode) |

## What was NOT verified (Qt6 required)

The Qt UI components compile only on a machine with Qt 6.5+ and
the `Widgets` module. The dev box does not have Qt6. The
following manual smoke test must be run on a Qt6-equipped machine
before tagging `v0.1.0`:

1. `cmake --build build --target client_sim` succeeds.
2. `./build/bin/client_sim` starts a window titled "client_sim".
3. `File -> Open Directory` opens a directory containing
   `Base.wz`, `Map.wz`, `Character.wz`, `String.wz` from a real
   MapleStory install.
4. The tree populates with one top-level entry per WZ file. Each
   row is expandable.
5. Expanding a `WzImage` row triggers a brief wait cursor, and
   sub-property rows appear once expansion completes.
6. Activating a row whose value is a `WzPng` (e.g.
   `Character/00002000.img/stand/0/canvas`) shows the image at
   1.0x zoom, centered in the right pane.
7. Mouse wheel zooms; `Ctrl+0` resets to 1.0x; click-and-drag
   pans the canvas.
8. Opening a 500 MB WZ directory does not exhaust RAM. The model
   extracts images only on expansion (verify with a process
   monitor or by simply expanding a few hundred images).

If any item fails, do not tag `v0.1.0`. Open an issue and link it
to the failing checklist item.

## Git tag

`v0.1.0` is intentionally **not** created in this change set. The
tag is created only after the smoke test above passes on a
Qt6-equipped machine.

## Files added or modified in this change set

New:

- `app/CMakeLists.txt`
- `app/main.cpp`
- `app/RgbaToPixmap.{h,cpp}`
- `app/ImageViewer.{h,cpp}`
- `app/WzTreeModel.{h,cpp}`
- `app/WzTreeView.{h,cpp}`
- `app/MainWindow.{h,cpp}`
- `tests/test_wz_structure/CMakeLists.txt`
- `tests/test_wz_structure/main.cpp`
- `tests/fixtures/wz/README.md`
- `docs/M1_report.md`
- `docs/superpowers/specs/2026-06-08-m1-finish-design.md`
- `docs/superpowers/plans/2026-06-08-m1-finish.md`

Modified:

- `CMakeLists.txt` (gated `add_subdirectory(app)`)
- `tests/CMakeLists.txt` (added `add_subdirectory(test_wz_structure)`)
- `wzlib/include/wzlib/WzImage.h` (added `file()` getter/setter)
- `wzlib/src/WzFile.cpp` (one-line `setFile(this)` in `indexImages`)

## Deviations from the spec / plan

- Spec said the spec doc lives at `docs/superpowers/specs/...`; we
  put it at `docs/superpowers/specs/2026-06-08-m1-finish-design.md`
  per the brainstorming skill convention. No spec change.
- Spec said `WzTreeModel` would own a `unique_ptr<TreeNode>` tree.
  It does, but a `parent()` back-pointer on `TreeNode` was avoided
  in favour of a linear `findOwner` search. This is documented in
  the source as an O(N) per-call cost; a follow-up can add the
  back-pointer if profiling demands it.
- The plan's Task 2 test code used GoogleTest, but the project
  uses a custom `wztest` framework (no GTest dependency, by
  design — see `tests/wztest/wztest.h`). The plan code was
  translated to `wztest` macros (`TEST`, `EXPECT_EQ`,
  `ASSERT_TRUE`, `EXPECT_TRUE`) and the CMake link target was
  switched from `GTest::gtest` to `wztest`. The five test cases
  themselves (decodeRgba byte order + WzNode::get on a synthetic
  tree) are unchanged. Real-WZ-fixture tests will be added in a
  follow-up, gated on user-provided fixture files.
- The plan's `WzTreeView` code used `wzlib::errorCodeToString`,
  but the actual function name is `wzlib::toString(ErrorCode)`
  (see `wzlib/include/wzlib/ErrorCode.h`). Fixed in
  `app/WzTreeView.cpp`.
- The plan's `WzTreeView::ensureExtracted` checked the
  `VoidResult` returned by `WzImage::tryExtract` with `if (!result)`,
  but `VoidResult` has no `operator bool()`. The actual call
  uses `if (result.isError())`. Fixed in `app/WzTreeView.cpp`.
