# G4F-Engine (Win64, MinGW-w64)

G4F-Engine is a lightweight C-first game engine for **Windows x64** with:
- Win32 window + input (no GLFW dependency)
- Direct2D/DirectWrite UI rendering for menus/HUD
- D3D11 3D foundation (procedural meshes + code-generated materials/textures; no asset files required)

## Build
Prereq: `g++` from **x86_64 MinGW-w64** in `PATH`.

Run:
- `build.bat`

Outputs:
- `out/lib/libg4f.a` (engine static lib)
- `out/bin/*.exe` (samples + engine tests)
- `out/bin/backrooms-tests/*.exe` (vendored Backrooms unit test suite)

## Where to start
- Engine API reference: `Reference.md`
- Development plan + rules: `Agents.md`
- Main C API header: `engine/include/g4f/g4f.h`

## Samples
- `samples/hello2d` - Direct2D text/rect bring-up
- `samples/backrooms_menu_smoke` - UI + image widgets (procedural bitmap)
- `samples/spin_cube` - D3D11 mesh/material/lighting + Direct2D UI overlay
