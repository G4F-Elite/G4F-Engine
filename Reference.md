# G4F-Engine (Win64, MinGW-w64) — Reference

## Goal
Lightweight engine for writing games in C/C++ on **Windows x64** with a **simple API**, strong **2D rendering** for menus/HUD, and **no GLFW dependency**.

Non-goals (intentionally **not** supported):
- Cross-platform support
- Editor GUI/TUI tooling (dev flow is text-only)
- 3D pipeline, 3D model importers

Notes on “textures”:
- The engine supports **2D bitmaps** (sprites/UI) as a practical requirement for menus and HUD.
- It does **not** provide a 3D texture/material system.

## Tech choices (Win64-only)
- Window + input: **Win32** (`CreateWindowEx`, message loop)
- 2D rendering: **Direct2D**
- Text: **DirectWrite**
- Image decoding: **WIC** (PNG/JPG/etc.)

## Repository layout
- `engine/include/g4f/g4f.h` — C API (stable surface)
- `engine/src/` — Win32 + D2D/DWrite/WIC implementation
- `samples/` — small runnable apps using the engine
- `tests/` — engine tests
- `Backrooms-master/` — upstream game used as smoke suite (primarily unit tests)

## Build (MinGW-w64)
Use `build.bat` at repo root:
- builds the engine
- builds samples
- builds and runs tests (engine + Backrooms test suite)

The build expects `g++` from **mingw-w64 x86_64** to be available in `PATH`.

Outputs:
- `out/bin/*.exe` — samples + tests
- `out/lib/libg4f.a` — engine static library

## Public API (high level)
The engine is “app-callback” oriented:
- create a window
- run a loop
- provide callbacks for init/update/render/shutdown

2D renderer focuses on:
- solid rects
- lines
- text
- bitmap blits

## Input model
The engine exposes key codes in a GLFW-like integer space for ergonomic porting of codebases that previously used GLFW **without linking GLFW**.

## Performance posture (Win64-first)
- Single-threaded main loop by default (predictable)
- Rendering batches internally (where possible)
- Minimal allocations per-frame; caller is encouraged to reuse buffers

## Current status
See `Agents.md` for the development plan, milestones, and “what to implement next”.

## Backrooms validation (no GLFW dependency)
`Backrooms-master/` is treated as a vendored reference project.
`build.bat` compiles and runs `Backrooms-master/tests/*.cpp` using header-only shims from `compat/include`:
- `compat/include/GLFW/glfw3.h` — keycode + API declarations (no library)
- `compat/include/glad/glad.h` — OpenGL type/prototype declarations (no loader)
