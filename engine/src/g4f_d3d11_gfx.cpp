#include "g4f_platform_d3d11.h"

#include "../include/g4f/g4f.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <d3dcompiler.h>

#include <cmath>
#include <cstring>
#include <cstdint>

namespace {

struct Mat4 {
    float m[16];
};

static Mat4 mat4Identity() {
    Mat4 out{};
    out.m[0] = 1.0f;
    out.m[5] = 1.0f;
    out.m[10] = 1.0f;
    out.m[15] = 1.0f;
    return out;
}

static Mat4 mat4Mul(const Mat4& a, const Mat4& b) {
    Mat4 out{};
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            out.m[r * 4 + c] =
                a.m[r * 4 + 0] * b.m[0 * 4 + c] +
                a.m[r * 4 + 1] * b.m[1 * 4 + c] +
                a.m[r * 4 + 2] * b.m[2 * 4 + c] +
                a.m[r * 4 + 3] * b.m[3 * 4 + c];
        }
    }
    return out;
}

static Mat4 mat4RotationY(float radians) {
    Mat4 out = mat4Identity();
    float c = std::cos(radians);
    float s = std::sin(radians);
    out.m[0] = c;
    out.m[2] = s;
    out.m[8] = -s;
    out.m[10] = c;
    return out;
}

static Mat4 mat4RotationX(float radians) {
    Mat4 out = mat4Identity();
    float c = std::cos(radians);
    float s = std::sin(radians);
    out.m[5] = c;
    out.m[6] = -s;
    out.m[9] = s;
    out.m[10] = c;
    return out;
}

static Mat4 mat4Translation(float x, float y, float z) {
    Mat4 out = mat4Identity();
    out.m[12] = x;
    out.m[13] = y;
    out.m[14] = z;
    return out;
}

static Mat4 mat4Perspective(float fovYRadians, float aspect, float zn, float zf) {
    Mat4 out{};
    float yScale = 1.0f / std::tan(fovYRadians * 0.5f);
    float xScale = yScale / aspect;
    out.m[0] = xScale;
    out.m[5] = yScale;
    out.m[10] = zf / (zf - zn);
    out.m[11] = 1.0f;
    out.m[14] = (-zn * zf) / (zf - zn);
    return out;
}

static float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static void rgbaU32ToFloat4(uint32_t rgba, float out[4]) {
    out[0] = (float)((rgba >> 24) & 0xFF) / 255.0f;
    out[1] = (float)((rgba >> 16) & 0xFF) / 255.0f;
    out[2] = (float)((rgba >> 8) & 0xFF) / 255.0f;
    out[3] = (float)((rgba) & 0xFF) / 255.0f;
    out[0] = clamp01(out[0]);
    out[1] = clamp01(out[1]);
    out[2] = clamp01(out[2]);
    out[3] = clamp01(out[3]);
}

static void safeRelease(IUnknown** ptr) {
    g4f_safe_release(ptr);
}

static HRESULT compileHlsl(
    const char* source,
    const char* entryPoint,
    const char* target,
    ID3DBlob** outBlob
) {
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    ID3DBlob* errors = nullptr;
    HRESULT hr = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entryPoint, target, flags, 0, outBlob, &errors);
    if (FAILED(hr) && errors) {
        OutputDebugStringA((const char*)errors->GetBufferPointer());
    }
    if (errors) errors->Release();
    return hr;
}

struct Vertex {
    float px, py, pz;
    float cr, cg, cb, ca;
};

} // namespace

static bool gfxCreateTargets(g4f_gfx* gfx, int w, int h) {
    if (!gfx || !gfx->swapChain || !gfx->device) return false;

    safeRelease((IUnknown**)&gfx->rtv);
    safeRelease((IUnknown**)&gfx->dsv);
    safeRelease((IUnknown**)&gfx->depthTex);

    HRESULT hr = gfx->swapChain->ResizeBuffers(0, (UINT)w, (UINT)h, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) return false;

    ID3D11Texture2D* backBuffer = nullptr;
    hr = gfx->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr) || !backBuffer) return false;

    hr = gfx->device->CreateRenderTargetView(backBuffer, nullptr, &gfx->rtv);
    backBuffer->Release();
    if (FAILED(hr) || !gfx->rtv) return false;

    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = (UINT)w;
    depthDesc.Height = (UINT)h;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = gfx->device->CreateTexture2D(&depthDesc, nullptr, &gfx->depthTex);
    if (FAILED(hr) || !gfx->depthTex) return false;

    hr = gfx->device->CreateDepthStencilView(gfx->depthTex, nullptr, &gfx->dsv);
    if (FAILED(hr) || !gfx->dsv) return false;

    gfx->cachedW = w;
    gfx->cachedH = h;
    return true;
}

static bool gfxEnsureSize(g4f_gfx* gfx) {
    if (!gfx || !gfx->window) return false;
    int w = 0, h = 0;
    g4f_window_get_size(gfx->window, &w, &h);
    w = (w <= 0) ? 1 : w;
    h = (h <= 0) ? 1 : h;
    if (w == gfx->cachedW && h == gfx->cachedH && gfx->rtv && gfx->dsv) return true;
    return gfxCreateTargets(gfx, w, h);
}

static bool gfxCreateShadersAndCube(g4f_gfx* gfx) {
    static const char* kShader = R"(
cbuffer CB0 : register(b0) { row_major float4x4 uMvp; };
struct VSIn { float3 pos : POSITION; float4 col : COLOR; };
struct PSIn { float4 pos : SV_Position; float4 col : COLOR; };
PSIn VSMain(VSIn i){
  PSIn o;
  o.pos = mul(float4(i.pos,1.0), uMvp);
  o.col = i.col;
  return o;
}
float4 PSMain(PSIn i) : SV_Target { return i.col; }
)";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    HRESULT hr = compileHlsl(kShader, "VSMain", "vs_5_0", &vsBlob);
    if (FAILED(hr) || !vsBlob) return false;
    hr = compileHlsl(kShader, "PSMain", "ps_5_0", &psBlob);
    if (FAILED(hr) || !psBlob) { vsBlob->Release(); return false; }

    hr = gfx->device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &gfx->vs);
    if (FAILED(hr)) { vsBlob->Release(); psBlob->Release(); return false; }
    hr = gfx->device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &gfx->ps);
    psBlob->Release();
    if (FAILED(hr)) { vsBlob->Release(); return false; }

    D3D11_INPUT_ELEMENT_DESC il[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = gfx->device->CreateInputLayout(il, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &gfx->inputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return false;

    // Cube (8 verts, 12 triangles).
    const Vertex verts[] = {
        {-1,-1,-1, 1,0,0,1}, {+1,-1,-1, 0,1,0,1}, {+1,+1,-1, 0,0,1,1}, {-1,+1,-1, 1,1,0,1},
        {-1,-1,+1, 1,0,1,1}, {+1,-1,+1, 0,1,1,1}, {+1,+1,+1, 1,1,1,1}, {-1,+1,+1, 0,0,0,1},
    };
    const uint16_t indices[] = {
        0,1,2, 0,2,3, // -Z
        4,6,5, 4,7,6, // +Z
        4,5,1, 4,1,0, // -Y
        3,2,6, 3,6,7, // +Y
        1,5,6, 1,6,2, // +X
        4,0,3, 4,3,7, // -X
    };
    gfx->indexCount = (UINT)(sizeof(indices) / sizeof(indices[0]));

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = (UINT)sizeof(verts);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData{};
    vbData.pSysMem = verts;
    hr = gfx->device->CreateBuffer(&vbDesc, &vbData, &gfx->vb);
    if (FAILED(hr) || !gfx->vb) return false;

    D3D11_BUFFER_DESC ibDesc{};
    ibDesc.ByteWidth = (UINT)sizeof(indices);
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibData{};
    ibData.pSysMem = indices;
    hr = gfx->device->CreateBuffer(&ibDesc, &ibData, &gfx->ib);
    if (FAILED(hr) || !gfx->ib) return false;

    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = (UINT)sizeof(Mat4);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = gfx->device->CreateBuffer(&cbDesc, nullptr, &gfx->cbMvp);
    if (FAILED(hr) || !gfx->cbMvp) return false;

    return true;
}

g4f_gfx* g4f_gfx_create(g4f_window* window) {
    if (!window) return nullptr;
    auto* gfx = new g4f_gfx();
    gfx->window = window;

    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.SampleDesc.Count = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 2;
    scd.OutputWindow = window->state.hwnd;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        &level,
        1,
        D3D11_SDK_VERSION,
        &scd,
        &gfx->swapChain,
        &gfx->device,
        nullptr,
        &gfx->ctx
    );
    if (FAILED(hr)) {
        g4f_gfx_destroy(gfx);
        return nullptr;
    }

    if (!gfxEnsureSize(gfx)) {
        g4f_gfx_destroy(gfx);
        return nullptr;
    }

    if (!gfxCreateShadersAndCube(gfx)) {
        g4f_gfx_destroy(gfx);
        return nullptr;
    }

    return gfx;
}

void g4f_gfx_destroy(g4f_gfx* gfx) {
    if (!gfx) return;
    safeRelease((IUnknown**)&gfx->cbMvp);
    safeRelease((IUnknown**)&gfx->ib);
    safeRelease((IUnknown**)&gfx->vb);
    safeRelease((IUnknown**)&gfx->inputLayout);
    safeRelease((IUnknown**)&gfx->ps);
    safeRelease((IUnknown**)&gfx->vs);
    safeRelease((IUnknown**)&gfx->dsv);
    safeRelease((IUnknown**)&gfx->depthTex);
    safeRelease((IUnknown**)&gfx->rtv);
    safeRelease((IUnknown**)&gfx->ctx);
    safeRelease((IUnknown**)&gfx->device);
    safeRelease((IUnknown**)&gfx->swapChain);
    delete gfx;
}

void g4f_gfx_begin(g4f_gfx* gfx, uint32_t clearRgba) {
    if (!gfx || !gfx->ctx) return;
    if (!gfxEnsureSize(gfx)) return;

    float clear[4];
    rgbaU32ToFloat4(clearRgba, clear);

    gfx->ctx->OMSetRenderTargets(1, &gfx->rtv, gfx->dsv);
    gfx->ctx->ClearRenderTargetView(gfx->rtv, clear);
    gfx->ctx->ClearDepthStencilView(gfx->dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = (FLOAT)gfx->cachedW;
    vp.Height = (FLOAT)gfx->cachedH;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    gfx->ctx->RSSetViewports(1, &vp);
}

void g4f_gfx_draw_debug_cube(g4f_gfx* gfx, float timeSeconds) {
    if (!gfx || !gfx->ctx) return;

    gfx->ctx->IASetInputLayout(gfx->inputLayout);
    gfx->ctx->VSSetShader(gfx->vs, nullptr, 0);
    gfx->ctx->PSSetShader(gfx->ps, nullptr, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    gfx->ctx->IASetVertexBuffers(0, 1, &gfx->vb, &stride, &offset);
    gfx->ctx->IASetIndexBuffer(gfx->ib, DXGI_FORMAT_R16_UINT, 0);
    gfx->ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    float aspect = (gfx->cachedH > 0) ? ((float)gfx->cachedW / (float)gfx->cachedH) : 1.0f;
    Mat4 proj = mat4Perspective(70.0f * 3.14159265f / 180.0f, aspect, 0.1f, 100.0f);
    Mat4 view = mat4Translation(0.0f, 0.0f, 4.0f);
    Mat4 rot = mat4Mul(mat4RotationY(timeSeconds * 0.8f), mat4RotationX(timeSeconds * 0.5f));
    Mat4 mvp = mat4Mul(mat4Mul(rot, view), proj);

    gfx->ctx->UpdateSubresource(gfx->cbMvp, 0, nullptr, &mvp, 0, 0);
    gfx->ctx->VSSetConstantBuffers(0, 1, &gfx->cbMvp);

    gfx->ctx->DrawIndexed(gfx->indexCount, 0, 0);
}

void g4f_gfx_end(g4f_gfx* gfx) {
    if (!gfx || !gfx->swapChain) return;
    gfx->swapChain->Present(1, 0);
}
