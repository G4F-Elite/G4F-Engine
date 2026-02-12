# G4F-Engine (Win64, MinGW-w64) - Reference

## Goal
Lightweight engine for writing games in C/C++ on **Windows x64** with a **simple API**, strong **2D rendering** for menus/HUD, and **no GLFW dependency**.

Non-goals (intentionally **not** supported):
- Cross-platform support
- Editor GUI/TUI tooling (dev flow is text-only)
- Asset pipelines that require external files (models/textures)

Assets and materials policy:
- 3D focus is planned/expected, but **not from files**: no loading `.fbx/.obj`, no "drop texture files into a folder".
- Any meshes/materials/textures are expected to be **generated in code** (procedural, hardcoded, embedded bytes, etc.).
- 2D supports **bitmaps for UI** (icons/panels/menus) as a practical requirement.

## Tech choices (Win64-only)
- Window + input: **Win32** (`CreateWindowEx`, message loop)
- 2D rendering: **Direct2D**
- Text: **DirectWrite**
- Image decoding: **WIC** (PNG/JPG/etc.)

## Repository layout
- `engine/include/g4f/g4f.h` - C API (stable surface)
- `engine/src/` - Win32 + D2D/DWrite/WIC implementation
- `samples/` - small runnable apps using the engine
- `tests/` - engine tests
- `Backrooms-master/` - upstream game used as smoke suite (primarily unit tests)

## Build (MinGW-w64)
Use `build.bat` at repo root:
- builds the engine
- builds samples
- builds and runs tests (engine + Backrooms test suite)

The build expects `g++` from **mingw-w64 x86_64** to be available in `PATH`.

Outputs:
- `out/bin/*.exe` - samples + tests
- `out/lib/libg4f.a` - engine static library

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

## Math helpers
Lightweight math for 3D (row-major matrices, row-vector `mul(v, m)` convention):
- `g4f_mat4_identity`, `g4f_mat4_mul`
- `g4f_mat4_translation`, `g4f_mat4_rotation_x`, `g4f_mat4_rotation_y`
- `g4f_mat4_perspective`, `g4f_mat4_look_at`

## UI (menus/panels)
Immediate-mode UI helper (no editor, all in code):
- Header: `engine/include/g4f/g4f_ui.h`
- Typical flow per-frame: `g4f_ui_begin` -> `g4f_ui_layout_begin` -> widgets -> `g4f_ui_end`
- Panels: `g4f_ui_panel_begin` / `g4f_ui_panel_end` (includes clipping)
- Scroll panels: `g4f_ui_panel_begin_scroll` (mouse wheel scroll)
- Keyboard nav (built-in): `UP/DOWN` (or `W/S`) focus, `TAB`/`Shift+TAB` cycle focus, `ENTER/SPACE` activate, `LEFT/RIGHT` (or `A/D`) adjust slider (hold `SHIFT` for fine step)
- Persistent UI state (optional): `g4f_ui_store_*` and keyed widgets `g4f_ui_checkbox_k` / `g4f_ui_slider_float_k`
- Text input: `g4f_ui_input_text_k` (uses per-frame OS text input)
- Text input mouse: click to set caret, drag to select, auto horizontal scroll for long lines
- Clipboard shortcuts in text input: `Ctrl+C` copy, `Ctrl+X` cut, `Ctrl+V` paste, `Enter`/`Esc` to finish editing
- Editing: `Ctrl+A` select all, `Shift+Left/Right` select, `Home/End`, `Ctrl+Left/Right` jump by word

## Input notes
- Text input comes from `WM_CHAR` and is available via `g4f_text_input_count` / `g4f_text_input_codepoint`.

## Text + clipping helpers
- Wrapped text: `g4f_draw_text_wrapped`, `g4f_measure_text_wrapped`
- Clip stack: `g4f_clip_push`, `g4f_clip_pop`

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

### 3D resources (no files)
This engine intentionally avoids loading textures/models from disk for 3D. Instead, you create resources from code:
- Textures: `g4f_gfx_texture_create_rgba8` (pixels come from memory)
- Materials: `g4f_gfx_material_create_unlit` (tint + optional texture)
- Meshes: `g4f_gfx_mesh_create_p3n3uv2`
- Draw: `g4f_gfx_draw_mesh`

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
See `Agents.md` for the development plan, milestones, and "what to implement next".

## Backrooms validation (no GLFW dependency)
`Backrooms-master/` is treated as a vendored reference project.
`build.bat` compiles and runs `Backrooms-master/tests/*.cpp` using header-only shims from `compat/include`:
- `compat/include/GLFW/glfw3.h` - keycode + API declarations (no library)
- `compat/include/glad/glad.h` - OpenGL type/prototype declarations (no loader)
