#include "g4f_platform_win32.h"

#include <unordered_map>
#include <string>

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

    ID2D1HwndRenderTarget* target = nullptr;
    ID2D1SolidColorBrush* brush = nullptr;

    std::unordered_map<int, IDWriteTextFormat*> textFormatsBySizePx;
};

struct g4f_bitmap {
    ID2D1Bitmap* bitmap = nullptr;
    int width = 0;
    int height = 0;
};

static HRESULT g4f_renderer_create_target(g4f_renderer* renderer) {
    RECT rc{};
    GetClientRect(renderer->window->state.hwnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU((UINT32)(rc.right - rc.left), (UINT32)(rc.bottom - rc.top));

    return renderer->d2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
        ),
        D2D1::HwndRenderTargetProperties(renderer->window->state.hwnd, size),
        &renderer->target
    );
}

g4f_renderer* g4f_renderer_create(g4f_window* window) {
    if (!window) return nullptr;
    auto* renderer = new g4f_renderer();
    renderer->window = window;

    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &renderer->d2dFactory);
    if (FAILED(hr)) { g4f_renderer_destroy(renderer); return nullptr; }

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&renderer->dwriteFactory);
    if (FAILED(hr)) { g4f_renderer_destroy(renderer); return nullptr; }

    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&renderer->wicFactory));
    if (FAILED(hr)) { g4f_renderer_destroy(renderer); return nullptr; }

    hr = g4f_renderer_create_target(renderer);
    if (FAILED(hr)) { g4f_renderer_destroy(renderer); return nullptr; }

    hr = renderer->target->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 1), &renderer->brush);
    if (FAILED(hr)) { g4f_renderer_destroy(renderer); return nullptr; }

    return renderer;
}

void g4f_renderer_destroy(g4f_renderer* renderer) {
    if (!renderer) return;

    for (auto& kv : renderer->textFormatsBySizePx) {
        if (kv.second) kv.second->Release();
    }
    renderer->textFormatsBySizePx.clear();

    g4f_safe_release((IUnknown**)&renderer->brush);
    g4f_safe_release((IUnknown**)&renderer->target);
    g4f_safe_release((IUnknown**)&renderer->wicFactory);
    g4f_safe_release((IUnknown**)&renderer->dwriteFactory);
    g4f_safe_release((IUnknown**)&renderer->d2dFactory);

    delete renderer;
}

static void g4f_renderer_ensure_size(g4f_renderer* renderer) {
    if (!renderer || !renderer->target) return;
    RECT rc{};
    GetClientRect(renderer->window->state.hwnd, &rc);
    UINT w = (UINT)std::max<LONG>(1, rc.right - rc.left);
    UINT h = (UINT)std::max<LONG>(1, rc.bottom - rc.top);
    renderer->target->Resize(D2D1::SizeU(w, h));
}

void g4f_renderer_begin(g4f_renderer* renderer) {
    if (!renderer || !renderer->target) return;
    g4f_renderer_ensure_size(renderer);
    renderer->target->BeginDraw();
    renderer->target->SetTransform(D2D1::Matrix3x2F::Identity());
}

void g4f_renderer_end(g4f_renderer* renderer) {
    if (!renderer || !renderer->target) return;
    renderer->target->EndDraw();
}

void g4f_renderer_clear(g4f_renderer* renderer, uint32_t rgba) {
    if (!renderer || !renderer->target) return;
    renderer->target->Clear(g4f_color_from_rgba_u32(rgba));
}

void g4f_draw_rect(g4f_renderer* renderer, g4f_rect_f rect, uint32_t rgba) {
    if (!renderer || !renderer->target || !renderer->brush) return;
    renderer->brush->SetColor(g4f_color_from_rgba_u32(rgba));
    D2D1_RECT_F r{rect.x, rect.y, rect.x + rect.w, rect.y + rect.h};
    renderer->target->FillRectangle(r, renderer->brush);
}

void g4f_draw_rect_outline(g4f_renderer* renderer, g4f_rect_f rect, float thickness, uint32_t rgba) {
    if (!renderer || !renderer->target || !renderer->brush) return;
    renderer->brush->SetColor(g4f_color_from_rgba_u32(rgba));
    D2D1_RECT_F r{rect.x, rect.y, rect.x + rect.w, rect.y + rect.h};
    renderer->target->DrawRectangle(r, renderer->brush, thickness);
}

void g4f_draw_line(g4f_renderer* renderer, float x1, float y1, float x2, float y2, float thickness, uint32_t rgba) {
    if (!renderer || !renderer->target || !renderer->brush) return;
    renderer->brush->SetColor(g4f_color_from_rgba_u32(rgba));
    renderer->target->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), renderer->brush, thickness);
}

static IDWriteTextFormat* g4f_get_text_format(g4f_renderer* renderer, int sizePx) {
    if (sizePx < 6) sizePx = 6;
    if (sizePx > 200) sizePx = 200;

    auto it = renderer->textFormatsBySizePx.find(sizePx);
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
    if (FAILED(hr) || !format) return nullptr;
    format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    renderer->textFormatsBySizePx.emplace(sizePx, format);
    return format;
}

void g4f_draw_text(g4f_renderer* renderer, const char* text_utf8, float x, float y, float size_px, uint32_t rgba) {
    if (!renderer || !renderer->target || !renderer->brush || !text_utf8) return;
    std::wstring text = g4f_utf8_to_wide(text_utf8);
    if (text.empty()) return;

    int sizePx = (int)(size_px + 0.5f);
    IDWriteTextFormat* format = g4f_get_text_format(renderer, sizePx);
    if (!format) return;

    renderer->brush->SetColor(g4f_color_from_rgba_u32(rgba));

    // Large layout box; DirectWrite needs a rect.
    D2D1_SIZE_F rtSize = renderer->target->GetSize();
    D2D1_RECT_F layout{ x, y, rtSize.width, rtSize.height };
    renderer->target->DrawText(text.c_str(), (UINT32)text.size(), format, layout, renderer->brush);
}

g4f_bitmap* g4f_bitmap_load(g4f_renderer* renderer, const char* path_utf8) {
    if (!renderer || !renderer->wicFactory || !renderer->target || !path_utf8) return nullptr;
    std::wstring path = g4f_utf8_to_wide(path_utf8);
    if (path.empty()) return nullptr;

    IWICBitmapDecoder* decoder = nullptr;
    HRESULT hr = renderer->wicFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr) || !decoder) return nullptr;

    IWICBitmapFrameDecode* frame = nullptr;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame) { decoder->Release(); return nullptr; }

    IWICFormatConverter* converter = nullptr;
    hr = renderer->wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr) || !converter) { frame->Release(); decoder->Release(); return nullptr; }

    hr = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr)) { converter->Release(); frame->Release(); decoder->Release(); return nullptr; }

    ID2D1Bitmap* bitmap = nullptr;
    hr = renderer->target->CreateBitmapFromWicBitmap(converter, nullptr, &bitmap);
    if (FAILED(hr) || !bitmap) { converter->Release(); frame->Release(); decoder->Release(); return nullptr; }

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

void g4f_bitmap_destroy(g4f_bitmap* bitmap) {
    if (!bitmap) return;
    g4f_safe_release((IUnknown**)&bitmap->bitmap);
    delete bitmap;
}

void g4f_draw_bitmap(g4f_renderer* renderer, const g4f_bitmap* bitmap, g4f_rect_f dst, float opacity) {
    if (!renderer || !renderer->target || !bitmap || !bitmap->bitmap) return;
    if (opacity < 0.0f) opacity = 0.0f;
    if (opacity > 1.0f) opacity = 1.0f;
    D2D1_RECT_F r{dst.x, dst.y, dst.x + dst.w, dst.y + dst.h};
    renderer->target->DrawBitmap(bitmap->bitmap, r, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, nullptr);
}
