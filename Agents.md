# Agents (dev plan + rules)

This file is the **source of truth** for how we develop this engine going forward.
If a future change request arrives without a similarly detailed plan, start by reading this file and producing an updated plan before coding.

## Principles
- Win64-only. Prefer Win32/Direct2D/DirectWrite/WIC over cross-platform deps.
- C-first API (easy to consume), with optional C++ sugar in samples only.
- "3D-first" long-term, with "2D-excellent" for menus/HUD/debug overlays.
- Avoid heavyweight dependencies; keep the build to `build.bat` + `g++`.
  - Prefer "generate in code" over asset files.

## Git workflow
- This repo must be a git repository.
- **Commit every logical change** (small, incremental commits).
- Commit messages: imperative, short (e.g. "Add Win32 window loop").

## Milestones
### M0 - Skeleton
- Engine folder layout
- `build.bat` building a sample
- Minimal error handling

### M1 - Window + time + input (Win32)
- Window creation (title, size, resizable)
- Main loop helpers
- Keyboard + mouse input (pressed/released/down)
- Mouse wheel delta

### M2 - 2D renderer (Direct2D)
- Begin/end frame
- Clear color
- Draw rect/line
- Text rendering via DirectWrite

### M3 - Bitmaps (WIC)
- Load PNG/JPG from file to `ID2D1Bitmap`
- Draw bitmap with scaling/opacity

### M4 - Backrooms smoke validation
- Build and run `Backrooms-master/tests/*.cpp` as a smoke suite
- Keep a GLFW/glad header shim (no actual GLFW dependency)

### M5 - 3D foundation (planned)
- D3D11 rendering backend (Win64)
- Procedural meshes (no model files)
- Code-generated materials (no texture files)
- Keep 2D UI renderer for menus/HUD

### M6 - Diagnostics (in progress)
- Thread-local last error string: `g4f_last_error()` / `g4f_clear_error()`
- Use it for all major `*_create()` failure paths (Win32/D2D/WIC/D3D11)
- Tests must verify error is set on invalid args / failed creation

Current (bootstrap) state:
- `g4f_gfx` provides a D3D11 swapchain + a built-in debug cube draw.
- `g4f_gfx` also supports code-generated meshes/materials/textures (unlit).
- `g4f_ctx3d` is the simplest 3D integration wrapper.
- `g4f_ui` supports TAB navigation and a usable `g4f_ui_input_text_k` (mouse caret/drag selection + horizontal scroll).
- `tests/math_tests.cpp` validates `g4f_mat4` helpers.
- Mouse capture uses Win32 Raw Input (`WM_INPUT`) for stable per-frame `dx/dy`.
- Most failing API calls set a useful reason retrievable via `g4f_last_error()`.

## Build outputs
- `out/lib/libg4f.a` - engine static library
- `out/bin/hello2d.exe` - sample
- `out/bin/backrooms_menu_smoke.exe` - menu smoke sample
- `out/bin/engine_keycodes_tests.exe` - engine test
- `out/bin/backrooms-tests/*.exe` - Backrooms test suite executables

## Coding rules (project-local)
- No one-letter variable names in new code (except indices in tight loops).
- No "magic global state" in public API; keep internals behind opaque handles.
- Prefer explicit error codes over exceptions in the C API.
- Keep public headers Windows-free; platform details stay in `engine/src`.
