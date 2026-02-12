#include "g4f_platform_win32.h"
#include "g4f_platform_d3d11.h"
#include "g4f_error_internal.h"

#include <unordered_map>
#include <string>
#include <vector>

namespace {

static D2D1_COLOR_F g4f_color_from_rgba_u32(uint32_t rgba) {
    float r = (float)((rgba >> 24) & 0xFF) / 255.0f;
    float g = (float)((rgba >> 16) & 0xFF) / 255.0f;
    float b = (float)((rgba >> 8) & 0xFF) / 255.0f;
    float a = (float)((rgba) & 0xFF) / 255.0f;
    return D2D1::ColorF(r, g, b, a);
}

} // namespace

struct g4f_renderer {
    g4f_window* window = nullptr;
    ID2D1Factory* d2dFactory = nullptr;
    IDWriteFactory* dwriteFactory = nullptr;
    IWICImagingFactory* wicFactory = nullptr;

    // 2D target variants:
    // - hwndTarget: standalone 2D app
    // - gfxContext: overlay over a D3D11 swapchain backbuffer (ctxTargetBitmap)
    ID2D1HwndRenderTarget* hwndTarget = nullptr;
    ID2D1Factory1* d2dFactory1 = nullptr;
    ID2D1Device* d2dDevice = nullptr;
    ID2D1DeviceContext* gfxContext = nullptr;
    ID2D1Bitmap1* ctxTargetBitmap = nullptr;
    g4f_gfx* gfx = nullptr;
    uint64_t boundBackbufferGeneration = 0;

    ID2D1SolidColorBrush* brush = nullptr;

    std::unordered_map<int, IDWriteTextFormat*> textFormatsBySizePx;
    int clipDepth = 0;
};

struct g4f_bitmap {
    ID2D1Bitmap* bitmap = nullptr;
    int width = 0;
    int height = 0;
};

static ID2D1RenderTarget* g4f_active_target(g4f_renderer* renderer) {
    if (!renderer) return nullptr;
    if (renderer->hwndTarget) return renderer->hwndTarget;
    if (renderer->gfxContext) return renderer->gfxContext;
    return nullptr;
}

static HRESULT g4f_renderer_create_hwnd_target(g4f_renderer* renderer) {
    RECT rc{};
    GetClientRect(renderer->window->state.hwnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU((UINT32)(rc.right - rc.left), (UINT32)(rc.bottom - rc.top));

    return renderer->d2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
        ),
        D2D1::HwndRenderTargetProperties(renderer->window->state.hwnd, size),
        &renderer->hwndTarget
    );
}

g4f_renderer* g4f_renderer_create(g4f_window* window) {
    if (!window) {
        g4f_set_last_error("g4f_renderer_create: window is null");
        return nullptr;
    }
    auto* renderer = new g4f_renderer();
    renderer->window = window;

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &renderer->d2dFactory);
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_renderer_create: D2D1CreateFactory failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&renderer->dwriteFactory);
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_renderer_create: DWriteCreateFactory failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&renderer->wicFactory));
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_renderer_create: WIC factory creation failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    hr = g4f_renderer_create_hwnd_target(renderer);
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_renderer_create: create HWND target failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    hr = renderer->hwndTarget->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &renderer->brush);
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_renderer_create: CreateSolidColorBrush failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    return renderer;
}

void g4f_renderer_destroy(g4f_renderer* renderer) {
    if (!renderer) return;

    for (auto& kv : renderer->textFormatsBySizePx) {
        if (kv.second) kv.second->Release();
    }
    renderer->textFormatsBySizePx.clear();

    g4f_safe_release((IUnknown**)&renderer->brush);
    g4f_safe_release((IUnknown**)&renderer->ctxTargetBitmap);
    g4f_safe_release((IUnknown**)&renderer->gfxContext);
    g4f_safe_release((IUnknown**)&renderer->d2dDevice);
    g4f_safe_release((IUnknown**)&renderer->d2dFactory1);
    g4f_safe_release((IUnknown**)&renderer->hwndTarget);
    g4f_safe_release((IUnknown**)&renderer->wicFactory);
    g4f_safe_release((IUnknown**)&renderer->dwriteFactory);
    g4f_safe_release((IUnknown**)&renderer->d2dFactory);

    delete renderer;
}

static void g4f_renderer_ensure_hwnd_size(g4f_renderer* renderer) {
    if (!renderer || !renderer->hwndTarget) return;
    RECT rc{};
    GetClientRect(renderer->window->state.hwnd, &rc);
    UINT w = (UINT)std::max<LONG>(1, rc.right - rc.left);
    UINT h = (UINT)std::max<LONG>(1, rc.bottom - rc.top);
    renderer->hwndTarget->Resize(D2D1::SizeU(w, h));
}

static HRESULT g4f_renderer_bind_gfx_backbuffer(g4f_renderer* renderer) {
    if (!renderer || !renderer->gfx || !renderer->gfxContext) return E_FAIL;

    g4f_safe_release((IUnknown**)&renderer->ctxTargetBitmap);

    IDXGISurface* surface = nullptr;
    HRESULT hr = renderer->gfx->swapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surface);
    if (FAILED(hr) || !surface) return FAILED(hr) ? hr : E_FAIL;

    D2D1_BITMAP_PROPERTIES1 props{};
    props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    // Swapchain backbuffers are typically treated as opaque; using IGNORE is the
    // most broadly compatible choice for D2D-on-DXGI interop.
    props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    props.bitmapOptions = (D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW);
    props.dpiX = 96.0f;
    props.dpiY = 96.0f;

    hr = renderer->gfxContext->CreateBitmapFromDxgiSurface(surface, &props, &renderer->ctxTargetBitmap);
    surface->Release();
    if (FAILED(hr) || !renderer->ctxTargetBitmap) return FAILED(hr) ? hr : E_FAIL;

    renderer->gfxContext->SetTarget(renderer->ctxTargetBitmap);
    renderer->boundBackbufferGeneration = renderer->gfx->backbufferGeneration;
    return S_OK;
}

void g4f_renderer_begin(g4f_renderer* renderer) {
    if (!renderer) return;
    if (renderer->hwndTarget) {
        g4f_renderer_ensure_hwnd_size(renderer);
        renderer->hwndTarget->BeginDraw();
        renderer->hwndTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        return;
    }
    if (renderer->gfxContext && renderer->gfx) {
        if (renderer->gfx->ctx) renderer->gfx->ctx->Flush();
        if (!renderer->ctxTargetBitmap || renderer->boundBackbufferGeneration != renderer->gfx->backbufferGeneration) {
            if (FAILED(g4f_renderer_bind_gfx_backbuffer(renderer))) return;
        }
        renderer->gfxContext->BeginDraw();
        renderer->gfxContext->SetTransform(D2D1::Matrix3x2F::Identity());
    }
}

void g4f_renderer_end(g4f_renderer* renderer) {
    if (!renderer) return;
    if (renderer->hwndTarget) {
        renderer->hwndTarget->EndDraw();
        return;
    }
    if (renderer->gfxContext) {
        renderer->gfxContext->EndDraw();
    }
}

void g4f_renderer_clear(g4f_renderer* renderer, uint32_t rgba) {
    if (!renderer) return;
    if (renderer->hwndTarget) {
        renderer->hwndTarget->Clear(g4f_color_from_rgba_u32(rgba));
        return;
    }
    if (renderer->gfxContext) {
        renderer->gfxContext->Clear(g4f_color_from_rgba_u32(rgba));
    }
}

void g4f_draw_rect(g4f_renderer* renderer, g4f_rect_f rect, uint32_t rgba) {
    if (!renderer || !renderer->brush) return;
    renderer->brush->SetColor(g4f_color_from_rgba_u32(rgba));
    D2D1_RECT_F r{rect.x, rect.y, rect.x + rect.w, rect.y + rect.h};
    if (renderer->hwndTarget) renderer->hwndTarget->FillRectangle(r, renderer->brush);
    else if (renderer->gfxContext) renderer->gfxContext->FillRectangle(r, renderer->brush);
}

void g4f_draw_rect_outline(g4f_renderer* renderer, g4f_rect_f rect, float thickness, uint32_t rgba) {
    if (!renderer || !renderer->brush) return;
    renderer->brush->SetColor(g4f_color_from_rgba_u32(rgba));
    D2D1_RECT_F r{rect.x, rect.y, rect.x + rect.w, rect.y + rect.h};
    if (renderer->hwndTarget) renderer->hwndTarget->DrawRectangle(r, renderer->brush, thickness);
    else if (renderer->gfxContext) renderer->gfxContext->DrawRectangle(r, renderer->brush, thickness);
}

void g4f_draw_line(g4f_renderer* renderer, float x1, float y1, float x2, float y2, float thickness, uint32_t rgba) {
    if (!renderer || !renderer->brush) return;
    renderer->brush->SetColor(g4f_color_from_rgba_u32(rgba));
    if (renderer->hwndTarget) renderer->hwndTarget->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), renderer->brush, thickness);
    else if (renderer->gfxContext) renderer->gfxContext->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), renderer->brush, thickness);
}

static IDWriteTextFormat* g4f_get_text_format(g4f_renderer* renderer, int sizePx, bool wrap, const char* contextUtf8) {
    if (!renderer || !renderer->dwriteFactory) {
        g4f_set_last_errorf("%s: renderer/dwriteFactory is null", contextUtf8 ? contextUtf8 : "g4f_text");
        return nullptr;
    }
    if (sizePx < 6) sizePx = 6;
    if (sizePx > 200) sizePx = 200;

    int key = sizePx * 2 + (wrap ? 1 : 0);
    auto it = renderer->textFormatsBySizePx.find(key);
    if (it != renderer->textFormatsBySizePx.end()) return it->second;

    IDWriteTextFormat* format = nullptr;
    HRESULT hr = renderer->dwriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        (FLOAT)sizePx,
        L"",
        &format
    );
    if (FAILED(hr) || !format) {
        g4f_set_last_hresult_error("g4f_text: CreateTextFormat failed", hr);
        return nullptr;
    }
    format->SetWordWrapping(wrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);

    renderer->textFormatsBySizePx.emplace(key, format);
    return format;
}

void g4f_draw_text(g4f_renderer* renderer, const char* text_utf8, float x, float y, float size_px, uint32_t rgba) {
    if (!renderer || !renderer->brush || !text_utf8) return;
    std::wstring text = g4f_utf8_to_wide(text_utf8);
    if (text.empty()) return;

    int sizePx = (int)(size_px + 0.5f);
    IDWriteTextFormat* format = g4f_get_text_format(renderer, sizePx, false, "g4f_draw_text");
    if (!format) return;

    renderer->brush->SetColor(g4f_color_from_rgba_u32(rgba));

    // Large layout box; DirectWrite needs a rect.
    D2D1_SIZE_F rtSize = renderer->hwndTarget ? renderer->hwndTarget->GetSize() : renderer->gfxContext->GetSize();
    D2D1_RECT_F layout{ x, y, rtSize.width, rtSize.height };
    if (renderer->hwndTarget) renderer->hwndTarget->DrawText(text.c_str(), (UINT32)text.size(), format, layout, renderer->brush);
    else if (renderer->gfxContext) renderer->gfxContext->DrawText(text.c_str(), (UINT32)text.size(), format, layout, renderer->brush);
}

void g4f_draw_text_wrapped(g4f_renderer* renderer, const char* text_utf8, g4f_rect_f bounds, float size_px, uint32_t rgba) {
    if (!renderer || !renderer->brush || !renderer->dwriteFactory || !text_utf8) return;
    std::wstring text = g4f_utf8_to_wide(text_utf8);
    if (text.empty()) return;

    int sizePx = (int)(size_px + 0.5f);
    IDWriteTextFormat* format = g4f_get_text_format(renderer, sizePx, true, "g4f_draw_text_wrapped");
    if (!format) return;

    renderer->brush->SetColor(g4f_color_from_rgba_u32(rgba));

    float w = (bounds.w > 0.0f) ? bounds.w : 1.0f;
    float h = (bounds.h > 0.0f) ? bounds.h : 1.0f;

    IDWriteTextLayout* layout = nullptr;
    HRESULT hr = renderer->dwriteFactory->CreateTextLayout(text.c_str(), (UINT32)text.size(), format, w, h, &layout);
    if (FAILED(hr) || !layout) {
        g4f_set_last_hresult_error("g4f_draw_text_wrapped: CreateTextLayout failed", hr);
        return;
    }

    ID2D1RenderTarget* target = g4f_active_target(renderer);
    if (target) target->DrawTextLayout(D2D1::Point2F(bounds.x, bounds.y), layout, renderer->brush);
    layout->Release();
}

g4f_bitmap* g4f_bitmap_load(g4f_renderer* renderer, const char* path_utf8) {
    if (!renderer || !renderer->wicFactory || !path_utf8) {
        g4f_set_last_error("g4f_bitmap_load: invalid args");
        return nullptr;
    }
    ID2D1RenderTarget* target = renderer->hwndTarget ? (ID2D1RenderTarget*)renderer->hwndTarget : (ID2D1RenderTarget*)renderer->gfxContext;
    if (!target) { g4f_set_last_error("g4f_bitmap_load: renderer has no active target"); return nullptr; }
    std::wstring path = g4f_utf8_to_wide(path_utf8);
    if (path.empty()) { g4f_set_last_error("g4f_bitmap_load: empty path"); return nullptr; }

    IWICBitmapDecoder* decoder = nullptr;
    HRESULT hr = renderer->wicFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr) || !decoder) { g4f_set_last_hresult_error("g4f_bitmap_load: CreateDecoderFromFilename failed", hr); return nullptr; }

    IWICBitmapFrameDecode* frame = nullptr;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame) { g4f_set_last_hresult_error("g4f_bitmap_load: decoder->GetFrame failed", hr); decoder->Release(); return nullptr; }

    IWICFormatConverter* converter = nullptr;
    hr = renderer->wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr) || !converter) { g4f_set_last_hresult_error("g4f_bitmap_load: CreateFormatConverter failed", hr); frame->Release(); decoder->Release(); return nullptr; }

    hr = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_bitmap_load: converter->Initialize failed", hr); converter->Release(); frame->Release(); decoder->Release(); return nullptr; }

    ID2D1Bitmap* bitmap = nullptr;
    hr = target->CreateBitmapFromWicBitmap(converter, nullptr, &bitmap);
    if (FAILED(hr) || !bitmap) { g4f_set_last_hresult_error("g4f_bitmap_load: CreateBitmapFromWicBitmap failed", hr); converter->Release(); frame->Release(); decoder->Release(); return nullptr; }

    UINT w = 0, h = 0;
    frame->GetSize(&w, &h);

    converter->Release();
    frame->Release();
    decoder->Release();

    auto* out = new g4f_bitmap();
    out->bitmap = bitmap;
    out->width = (int)w;
    out->height = (int)h;
    return out;
}

g4f_bitmap* g4f_bitmap_create_rgba8(g4f_renderer* renderer, int width, int height, const void* rgbaPixels, int rowPitchBytes) {
    if (!renderer || !rgbaPixels) { g4f_set_last_error("g4f_bitmap_create_rgba8: invalid args"); return nullptr; }
    if (width <= 0 || height <= 0) { g4f_set_last_error("g4f_bitmap_create_rgba8: invalid size"); return nullptr; }

    ID2D1RenderTarget* target = g4f_active_target(renderer);
    if (!target) { g4f_set_last_error("g4f_bitmap_create_rgba8: renderer has no active target"); return nullptr; }

    int srcPitch = rowPitchBytes;
    if (srcPitch <= 0) srcPitch = width * 4;

    std::vector<uint8_t> premulBgra;
    premulBgra.resize((size_t)width * (size_t)height * 4);

    const uint8_t* src = (const uint8_t*)rgbaPixels;
    for (int y = 0; y < height; y++) {
        const uint8_t* row = src + (size_t)y * (size_t)srcPitch;
        uint8_t* dst = premulBgra.data() + (size_t)y * (size_t)width * 4;
        for (int x = 0; x < width; x++) {
            uint8_t r = row[x * 4 + 0];
            uint8_t g = row[x * 4 + 1];
            uint8_t b = row[x * 4 + 2];
            uint8_t a = row[x * 4 + 3];

            uint32_t ap = (uint32_t)a;
            dst[x * 4 + 0] = (uint8_t)((uint32_t)b * ap / 255u);
            dst[x * 4 + 1] = (uint8_t)((uint32_t)g * ap / 255u);
            dst[x * 4 + 2] = (uint8_t)((uint32_t)r * ap / 255u);
            dst[x * 4 + 3] = a;
        }
    }

    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    ID2D1Bitmap* bitmap = nullptr;
    HRESULT hr = target->CreateBitmap(D2D1::SizeU((UINT32)width, (UINT32)height), premulBgra.data(), (UINT32)(width * 4), &props, &bitmap);
    if (FAILED(hr) || !bitmap) { g4f_set_last_hresult_error("g4f_bitmap_create_rgba8: CreateBitmap failed", hr); return nullptr; }

    auto* out = new g4f_bitmap();
    out->bitmap = bitmap;
    out->width = width;
    out->height = height;
    return out;
}

void g4f_bitmap_destroy(g4f_bitmap* bitmap) {
    if (!bitmap) return;
    g4f_safe_release((IUnknown**)&bitmap->bitmap);
    delete bitmap;
}

void g4f_bitmap_get_size(const g4f_bitmap* bitmap, int* width, int* height) {
    if (width) *width = bitmap ? bitmap->width : 0;
    if (height) *height = bitmap ? bitmap->height : 0;
}

void g4f_draw_bitmap(g4f_renderer* renderer, const g4f_bitmap* bitmap, g4f_rect_f dst, float opacity) {
    if (!renderer || !bitmap || !bitmap->bitmap) return;
    if (opacity < 0.0f) opacity = 0.0f;
    if (opacity > 1.0f) opacity = 1.0f;
    D2D1_RECT_F r{dst.x, dst.y, dst.x + dst.w, dst.y + dst.h};
    if (renderer->hwndTarget) renderer->hwndTarget->DrawBitmap(bitmap->bitmap, r, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, nullptr);
    else if (renderer->gfxContext) renderer->gfxContext->DrawBitmap(bitmap->bitmap, r, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, nullptr);
}

void g4f_draw_round_rect(g4f_renderer* renderer, g4f_rect_f rect, float radius, uint32_t rgba) {
    if (!renderer || !renderer->brush) return;
    if (radius < 0.0f) radius = 0.0f;
    renderer->brush->SetColor(g4f_color_from_rgba_u32(rgba));
    D2D1_ROUNDED_RECT rr{};
    rr.rect = D2D1::RectF(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
    rr.radiusX = radius;
    rr.radiusY = radius;
    if (renderer->hwndTarget) renderer->hwndTarget->FillRoundedRectangle(rr, renderer->brush);
    else if (renderer->gfxContext) renderer->gfxContext->FillRoundedRectangle(rr, renderer->brush);
}

void g4f_draw_round_rect_outline(g4f_renderer* renderer, g4f_rect_f rect, float radius, float thickness, uint32_t rgba) {
    if (!renderer || !renderer->brush) return;
    if (radius < 0.0f) radius = 0.0f;
    renderer->brush->SetColor(g4f_color_from_rgba_u32(rgba));
    D2D1_ROUNDED_RECT rr{};
    rr.rect = D2D1::RectF(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
    rr.radiusX = radius;
    rr.radiusY = radius;
    if (renderer->hwndTarget) renderer->hwndTarget->DrawRoundedRectangle(rr, renderer->brush, thickness);
    else if (renderer->gfxContext) renderer->gfxContext->DrawRoundedRectangle(rr, renderer->brush, thickness);
}

void g4f_measure_text(g4f_renderer* renderer, const char* text_utf8, float size_px, float* out_w, float* out_h) {
    if (out_w) *out_w = 0.0f;
    if (out_h) *out_h = 0.0f;
    if (!renderer || !text_utf8) return;
    std::wstring text = g4f_utf8_to_wide(text_utf8);
    if (text.empty()) return;
    int sizePx = (int)(size_px + 0.5f);
    IDWriteTextFormat* format = g4f_get_text_format(renderer, sizePx, false, "g4f_measure_text");
    if (!format) return;

    IDWriteTextLayout* layout = nullptr;
    HRESULT hr = renderer->dwriteFactory->CreateTextLayout(text.c_str(), (UINT32)text.size(), format, 10000.0f, 10000.0f, &layout);
    if (FAILED(hr) || !layout) {
        g4f_set_last_hresult_error("g4f_measure_text: CreateTextLayout failed", hr);
        return;
    }
    DWRITE_TEXT_METRICS m{};
    layout->GetMetrics(&m);
    layout->Release();
    if (out_w) *out_w = m.widthIncludingTrailingWhitespace;
    if (out_h) *out_h = m.height;
}

void g4f_measure_text_wrapped(g4f_renderer* renderer, const char* text_utf8, float size_px, float max_w, float max_h, float* out_w, float* out_h) {
    if (out_w) *out_w = 0.0f;
    if (out_h) *out_h = 0.0f;
    if (!renderer || !renderer->dwriteFactory || !text_utf8) return;
    std::wstring text = g4f_utf8_to_wide(text_utf8);
    if (text.empty()) return;
    int sizePx = (int)(size_px + 0.5f);
    IDWriteTextFormat* format = g4f_get_text_format(renderer, sizePx, true, "g4f_measure_text_wrapped");
    if (!format) return;

    float w = (max_w > 0.0f) ? max_w : 1.0f;
    float h = (max_h > 0.0f) ? max_h : 1.0f;

    IDWriteTextLayout* layout = nullptr;
    HRESULT hr = renderer->dwriteFactory->CreateTextLayout(text.c_str(), (UINT32)text.size(), format, w, h, &layout);
    if (FAILED(hr) || !layout) {
        g4f_set_last_hresult_error("g4f_measure_text_wrapped: CreateTextLayout failed", hr);
        return;
    }
    DWRITE_TEXT_METRICS m{};
    layout->GetMetrics(&m);
    layout->Release();
    if (out_w) *out_w = m.widthIncludingTrailingWhitespace;
    if (out_h) *out_h = m.height;
}

void g4f_clip_push(g4f_renderer* renderer, g4f_rect_f rect) {
    if (!renderer) return;
    ID2D1RenderTarget* target = g4f_active_target(renderer);
    if (!target) return;
    D2D1_RECT_F r{rect.x, rect.y, rect.x + rect.w, rect.y + rect.h};
    target->PushAxisAlignedClip(r, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    renderer->clipDepth += 1;
}

void g4f_clip_pop(g4f_renderer* renderer) {
    if (!renderer) return;
    ID2D1RenderTarget* target = g4f_active_target(renderer);
    if (!target) return;
    if (renderer->clipDepth <= 0) return;
    target->PopAxisAlignedClip();
    renderer->clipDepth -= 1;
}

g4f_renderer* g4f_renderer_create_for_gfx(g4f_gfx* gfx) {
    if (!gfx || !gfx->window || !gfx->device || !gfx->swapChain) {
        g4f_set_last_error("g4f_renderer_create_for_gfx: invalid gfx");
        return nullptr;
    }
    auto* renderer = new g4f_renderer();
    renderer->window = gfx->window;
    renderer->gfx = gfx;

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &renderer->d2dFactory);
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_renderer_create_for_gfx: D2D1CreateFactory failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &renderer->d2dFactory1);
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_renderer_create_for_gfx: D2D1CreateFactory(1) failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&renderer->dwriteFactory);
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_renderer_create_for_gfx: DWriteCreateFactory failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&renderer->wicFactory));
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_renderer_create_for_gfx: WIC factory creation failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    IDXGIDevice* dxgiDevice = nullptr;
    hr = gfx->device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr) || !dxgiDevice) { g4f_set_last_hresult_error("g4f_renderer_create_for_gfx: QueryInterface(IDXGIDevice) failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    hr = renderer->d2dFactory1->CreateDevice(dxgiDevice, &renderer->d2dDevice);
    dxgiDevice->Release();
    if (FAILED(hr) || !renderer->d2dDevice) { g4f_set_last_hresult_error("g4f_renderer_create_for_gfx: CreateDevice failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    hr = renderer->d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &renderer->gfxContext);
    if (FAILED(hr) || !renderer->gfxContext) { g4f_set_last_hresult_error("g4f_renderer_create_for_gfx: CreateDeviceContext failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    hr = renderer->gfxContext->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &renderer->brush);
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_renderer_create_for_gfx: CreateSolidColorBrush failed", hr); g4f_renderer_destroy(renderer); return nullptr; }

    // Bind backbuffer now.
    hr = g4f_renderer_bind_gfx_backbuffer(renderer);
    if (FAILED(hr)) { g4f_set_last_hresult_error("g4f_renderer_create_for_gfx: bind backbuffer failed", hr); g4f_renderer_destroy(renderer); return nullptr; }
    return renderer;
}
