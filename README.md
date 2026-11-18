# TechEngine

TechEngine is a client/server 3D game engine written in C++20. It has a data-oriented core
and a physically grounded renderer, and it targets Windows first and Linux second.

The engine is built as static libraries with a strict link order, under `engine/`:
`base`, `platform`, `core`, `client` and `app`. The executables live in `apps/` (the
`runtime` and the `editor`), and `sdk/` is the public surface that game scripts compile
against.

## Clone the vault

The design notes, ADRs and sprint plans live in a separate repository, the vault. Clone it
into `docs/` at the root of this checkout:

```bash
git clone git@github.com:techattackteam/TechEngine-vault.git docs
```

Nothing in the build needs `docs/`, and nothing warns you when it is missing.

## Build and test

Every build goes through `CMakePresets.json`, so a local build runs the same presets as CI.
On Windows (MSVC, Ninja Multi-Config):

```bash
cmake --preset windows
cmake --build --preset windows-debug
ctest --preset windows-debug
```

On Linux (Clang, Ninja), each configuration has its own configure preset:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

`CMakePresets.json` lists the other configurations: release, sanitizer and profiling builds.
