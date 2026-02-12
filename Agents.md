# Agents (dev plan + rules)

This file is the **source of truth** for how we develop this engine going forward.
If a future change request arrives without a similarly detailed plan, start by reading this file and producing an updated plan before coding.

## Principles
- Win64-only. Prefer Win32/Direct2D/DirectWrite/WIC over cross-platform deps.
- C-first API (easy to consume), with optional C++ sugar in samples only.
- “2D-first”: menus/HUD/debug overlays are the priority.
- Avoid heavyweight dependencies; keep the build to `build.bat` + `g++`.

## Git workflow
- This repo must be a git repository.
- **Commit every logical change** (small, incremental commits).
- Commit messages: imperative, short (e.g. “Add Win32 window loop”).

## Milestones
### M0 — Skeleton
- Engine folder layout
- `build.bat` building a sample
- Minimal error handling

### M1 — Window + time + input (Win32)
- Window creation (title, size, resizable)
- Main loop helpers
- Keyboard + mouse input (pressed/released/down)
- Mouse wheel delta

### M2 — 2D renderer (Direct2D)
- Begin/end frame
- Clear color
- Draw rect/line
- Text rendering via DirectWrite

### M3 — Bitmaps (WIC)
- Load PNG/JPG from file to `ID2D1Bitmap`
- Draw bitmap with scaling/opacity

### M4 — Backrooms smoke validation
- Build and run `Backrooms-master/tests/*.cpp` as a smoke suite
- Keep a GLFW/glad header shim (no actual GLFW dependency)

## Build outputs
- `out/lib/libg4f.a` — engine static library
- `out/bin/hello2d.exe` — sample
- `out/bin/backrooms_menu_smoke.exe` — menu smoke sample
- `out/bin/engine_keycodes_tests.exe` — engine test
- `out/bin/backrooms-tests/*.exe` — Backrooms test suite executables

## Coding rules (project-local)
- No one-letter variable names in new code (except indices in tight loops).
- No “magic global state” in public API; keep internals behind opaque handles.
- Prefer explicit error codes over exceptions in the C API.
- Keep public headers Windows-free; platform details stay in `engine/src`.
