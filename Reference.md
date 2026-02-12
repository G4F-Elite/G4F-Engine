# G4F-Engine (Win64, MinGW-w64) — Reference

## Goal
Lightweight engine for writing games in C/C++ on **Windows x64** with a **simple API**, strong **2D rendering** for menus/HUD, and **no GLFW dependency**.

Non-goals (intentionally **not** supported):
- Cross-platform support
- Editor GUI/TUI tooling (dev flow is text-only)
- Asset pipelines that require external files (models/textures)

Assets and materials policy:
- 3D focus is planned/expected, but **not from files**: no loading `.fbx/.obj`, no “drop texture files into a folder”.
- Any meshes/materials/textures are expected to be **generated in code** (procedural, hardcoded, embedded bytes, etc.).
- 2D supports **bitmaps for UI** (icons/panels/menus) as a practical requirement.

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

## Quickstart (simplest usage)
Minimal app using the high-level context:
```cpp
#include "g4f/g4f.h"
int main(){
  g4f_window_desc wd{"My Game", 1280, 720, 1};
  g4f_ctx* ctx = g4f_ctx_create(&wd);
  while(g4f_ctx_poll(ctx)){
    g4f_window* win = g4f_ctx_window(ctx);
    g4f_renderer* r = g4f_ctx_renderer(ctx);
    if(g4f_key_pressed(win, G4F_KEY_ESCAPE)) g4f_window_request_close(win);
    g4f_frame_begin(ctx, g4f_rgba_u32(18,18,22,255));
    g4f_draw_text(r, "Hello", 40, 40, 24, g4f_rgba_u32(255,255,255,255));
    g4f_frame_end(ctx);
  }
  g4f_ctx_destroy(ctx);
}
```

## Public API (high level)
Two levels:
- Low-level: `g4f_app` + `g4f_window` + `g4f_renderer`
- High-level: `g4f_ctx` + `g4f_frame_begin/end` (recommended for most apps)

## UI (menus/panels)
Immediate-mode UI helper (no editor, all in code):
- Header: `engine/include/g4f/g4f_ui.h`
- Typical flow per-frame: `g4f_ui_begin` → `g4f_ui_layout_begin` → widgets → `g4f_ui_end`

## 3D Quickstart (D3D11 bring-up)
Minimal 3D loop (no asset files; shaders/geometry are embedded/generated):
```cpp
#include "g4f/g4f.h"
int main(){
  g4f_window_desc wd{"My 3D Game", 1280, 720, 1};
  g4f_ctx3d* ctx = g4f_ctx3d_create(&wd);
  while(g4f_ctx3d_poll(ctx)){
    g4f_window* win = g4f_ctx3d_window(ctx);
    g4f_gfx* gfx = g4f_ctx3d_gfx(ctx);
    if(g4f_key_pressed(win, G4F_KEY_ESCAPE)) g4f_window_request_close(win);
    g4f_frame3d_begin(ctx, g4f_rgba_u32(14,14,18,255));
    g4f_gfx_draw_debug_cube(gfx, (float)g4f_ctx3d_time(ctx));
    g4f_frame3d_end(ctx);
  }
  g4f_ctx3d_destroy(ctx);
}
```

### 3D + UI overlay
For menus/panels/HUD on top of 3D, create a UI renderer bound to the swapchain:
- `g4f_renderer_create_for_gfx(g4f_gfx*)`
- then call the regular `g4f_draw_*` functions between your 3D draw and `g4f_gfx_end()`/`g4f_frame3d_end()`

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
