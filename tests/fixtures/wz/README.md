# WZ test fixtures

This directory is intentionally empty in the repository. Drop a
sample MapleStory install's `.wz` files here to enable the
real-WZ-fixture tests in `tests/test_wz_structure/`.

## What to drop

A minimum useful set:

- `Base.wz`
- `Map.wz` (optional; can be a small one)
- `Character.wz` (optional; can be a small one)
- `String.wz` (optional; enables the StringLinker tests)

KMS, GMS, JMS, MSEA, and Beta samples are all supported by wzlib;
the auto-detect in `WzStructure::open` picks the right PKG version.

## Warnings

- `.wz` files are copyrighted by Nexon / NEXON Korea / etc. Do
  **not** commit them to the repository. They are ignored by
  `.gitignore` (the `*.wz` rule). They are also too big to push
  to GitHub even if you tried.
- The fixture tests skip themselves when the directory is empty,
  so the absence of files here does not break the build.
