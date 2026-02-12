#include "g4f_platform_d3d11.h"
#include "g4f_error_internal.h"

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
#include <vector>

namespace {

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

static void vec3Normalize(float v[3]) {
    float len2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (len2 <= 0.0f) { v[0] = 0.0f; v[1] = -1.0f; v[2] = 0.0f; return; }
    float invLen = 1.0f / std::sqrt(len2);
    v[0] *= invLen;
    v[1] *= invLen;
    v[2] *= invLen;
}

static void safeRelease(IUnknown** ptr) {
    g4f_safe_release(ptr);
}

static g4f_mat4 mat4NormalMatrix(g4f_mat4 model) {
    // For row-vector convention: n_world = n_model * (M^-1)^T.
    // We only use the upper-left 3x3 of M (ignore translation).
    const float a00 = model.m[0];
    const float a01 = model.m[1];
    const float a02 = model.m[2];
    const float a10 = model.m[4];
    const float a11 = model.m[5];
    const float a12 = model.m[6];
    const float a20 = model.m[8];
    const float a21 = model.m[9];
    const float a22 = model.m[10];

    const float det =
        a00 * (a11 * a22 - a12 * a21) -
        a01 * (a10 * a22 - a12 * a20) +
        a02 * (a10 * a21 - a11 * a20);

    if (std::fabs(det) <= 1e-8f) {
        return g4f_mat4_identity();
    }

    const float invDet = 1.0f / det;

    // Inverse of 3x3 (row-major).
    const float inv00 = (a11 * a22 - a12 * a21) * invDet;
    const float inv01 = (a02 * a21 - a01 * a22) * invDet;
    const float inv02 = (a01 * a12 - a02 * a11) * invDet;
    const float inv10 = (a12 * a20 - a10 * a22) * invDet;
    const float inv11 = (a00 * a22 - a02 * a20) * invDet;
    const float inv12 = (a02 * a10 - a00 * a12) * invDet;
    const float inv20 = (a10 * a21 - a11 * a20) * invDet;
    const float inv21 = (a01 * a20 - a00 * a21) * invDet;
    const float inv22 = (a00 * a11 - a01 * a10) * invDet;

    // Normal matrix N = (M^-1)^T.
    g4f_mat4 out = g4f_mat4_identity();
    out.m[0] = inv00;
    out.m[1] = inv10;
    out.m[2] = inv20;
    out.m[4] = inv01;
    out.m[5] = inv11;
    out.m[6] = inv21;
    out.m[8] = inv02;
    out.m[9] = inv12;
    out.m[10] = inv22;
    return out;
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

struct CbMaterial {
    g4f_mat4 mvp;
    float tint[4];
    float hasTex;
    float pad[3];
    g4f_mat4 model;
    g4f_mat4 normal;
    float lightDir[4];
    float lightColor[4];
    float ambientColor[4];
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
    gfx->backbufferGeneration += 1;
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
    cbDesc.ByteWidth = (UINT)sizeof(g4f_mat4);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = gfx->device->CreateBuffer(&cbDesc, nullptr, &gfx->cbMvp);
    if (FAILED(hr) || !gfx->cbMvp) return false;

    return true;
}

static bool gfxCreateDefaultStates(g4f_gfx* gfx) {
    if (!gfx || !gfx->device) return false;

    D3D11_RASTERIZER_DESC rs{};
    rs.FillMode = D3D11_FILL_SOLID;
    rs.CullMode = D3D11_CULL_BACK;
    rs.FrontCounterClockwise = FALSE;
    rs.DepthClipEnable = TRUE;
    HRESULT hr = gfx->device->CreateRasterizerState(&rs, &gfx->rsCullBack);
    if (FAILED(hr) || !gfx->rsCullBack) return false;

    rs.CullMode = D3D11_CULL_NONE;
    hr = gfx->device->CreateRasterizerState(&rs, &gfx->rsCullNone);
    if (FAILED(hr) || !gfx->rsCullNone) return false;

    rs.CullMode = D3D11_CULL_FRONT;
    hr = gfx->device->CreateRasterizerState(&rs, &gfx->rsCullFront);
    if (FAILED(hr) || !gfx->rsCullFront) return false;

    D3D11_DEPTH_STENCIL_DESC ds{};
    ds.DepthEnable = TRUE;
    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc = D3D11_COMPARISON_LESS;
    ds.StencilEnable = FALSE;
    hr = gfx->device->CreateDepthStencilState(&ds, &gfx->dsDepthLess);
    if (FAILED(hr) || !gfx->dsDepthLess) return false;

    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    hr = gfx->device->CreateDepthStencilState(&ds, &gfx->dsDepthLessNoWrite);
    if (FAILED(hr) || !gfx->dsDepthLessNoWrite) return false;

    D3D11_DEPTH_STENCIL_DESC dsOff{};
    dsOff.DepthEnable = FALSE;
    dsOff.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsOff.DepthFunc = D3D11_COMPARISON_ALWAYS;
    dsOff.StencilEnable = FALSE;
    hr = gfx->device->CreateDepthStencilState(&dsOff, &gfx->dsDisabled);
    if (FAILED(hr) || !gfx->dsDisabled) return false;

    D3D11_BLEND_DESC bs{};
    bs.AlphaToCoverageEnable = FALSE;
    bs.IndependentBlendEnable = FALSE;
    D3D11_RENDER_TARGET_BLEND_DESC rt{};
    rt.BlendEnable = FALSE;
    rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    bs.RenderTarget[0] = rt;
    hr = gfx->device->CreateBlendState(&bs, &gfx->bsOpaque);
    if (FAILED(hr) || !gfx->bsOpaque) return false;

    D3D11_BLEND_DESC bsA{};
    bsA.AlphaToCoverageEnable = FALSE;
    bsA.IndependentBlendEnable = FALSE;
    D3D11_RENDER_TARGET_BLEND_DESC rta{};
    rta.BlendEnable = TRUE;
    rta.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    rta.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rta.BlendOp = D3D11_BLEND_OP_ADD;
    rta.SrcBlendAlpha = D3D11_BLEND_ONE;
    rta.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    rta.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rta.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    bsA.RenderTarget[0] = rta;
    hr = gfx->device->CreateBlendState(&bsA, &gfx->bsAlpha);
    if (FAILED(hr) || !gfx->bsAlpha) return false;

    return true;
}

static bool gfxCreateUnlitPipeline(g4f_gfx* gfx) {
    static const char* kShader = R"(
cbuffer CB0 : register(b0) {
  row_major float4x4 uMvp;
  float4 uTint;
  float uHasTex;
  float3 _pad;
  row_major float4x4 uModel;
  row_major float4x4 uNormal;
  float4 uLightDir;
  float4 uLightColor;
  float4 uAmbientColor;
};
Texture2D uTex0 : register(t0);
SamplerState uSamp0 : register(s0);
struct VSIn { float3 pos : POSITION; float3 n : NORMAL; float2 uv : TEXCOORD0; };
struct PSIn { float4 pos : SV_Position; float2 uv : TEXCOORD0; float3 n : NORMAL; };
PSIn VSMain(VSIn i){
  PSIn o;
  o.pos = mul(float4(i.pos,1.0), uMvp);
  o.uv = i.uv;
  o.n = mul(float4(i.n, 0.0), uNormal).xyz;
  return o;
}
float4 PSUnlit(PSIn i) : SV_Target {
  float4 c = uTint;
  if (uHasTex > 0.5) c *= uTex0.Sample(uSamp0, i.uv);
  return c;
}
float4 PSLit(PSIn i) : SV_Target {
  float4 base = uTint;
  if (uHasTex > 0.5) base *= uTex0.Sample(uSamp0, i.uv);
  float3 n = normalize(i.n);
  float3 l = normalize(-uLightDir.xyz);
  float ndl = saturate(dot(n, l));
  float3 lit = base.rgb * (uAmbientColor.rgb + ndl * uLightColor.rgb);
  return float4(lit, base.a);
}
)";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    HRESULT hr = compileHlsl(kShader, "VSMain", "vs_5_0", &vsBlob);
    if (FAILED(hr) || !vsBlob) return false;
    hr = compileHlsl(kShader, "PSUnlit", "ps_5_0", &psBlob);
    if (FAILED(hr) || !psBlob) { vsBlob->Release(); return false; }

    hr = gfx->device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &gfx->vsUnlit);
    if (FAILED(hr)) { vsBlob->Release(); psBlob->Release(); return false; }
    hr = gfx->device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &gfx->psUnlit);
    psBlob->Release();
    if (FAILED(hr)) { vsBlob->Release(); return false; }

    ID3DBlob* psLitBlob = nullptr;
    hr = compileHlsl(kShader, "PSLit", "ps_5_0", &psLitBlob);
    if (FAILED(hr) || !psLitBlob) { vsBlob->Release(); return false; }
    hr = gfx->device->CreatePixelShader(psLitBlob->GetBufferPointer(), psLitBlob->GetBufferSize(), nullptr, &gfx->psLit);
    psLitBlob->Release();
    if (FAILED(hr) || !gfx->psLit) { vsBlob->Release(); return false; }

    D3D11_INPUT_ELEMENT_DESC il[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = gfx->device->CreateInputLayout(il, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &gfx->ilUnlit);
    vsBlob->Release();
    if (FAILED(hr) || !gfx->ilUnlit) return false;

    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = (UINT)sizeof(CbMaterial);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = gfx->device->CreateBuffer(&cbDesc, nullptr, &gfx->cbUnlit);
    if (FAILED(hr) || !gfx->cbUnlit) return false;

    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samp.MinLOD = 0;
    samp.MaxLOD = D3D11_FLOAT32_MAX;
    hr = gfx->device->CreateSamplerState(&samp, &gfx->sampLinearClamp);
    if (FAILED(hr) || !gfx->sampLinearClamp) return false;

    return true;
}

g4f_gfx* g4f_gfx_create(g4f_window* window) {
    if (!window) {
        g4f_set_last_error("g4f_gfx_create: window is null");
        return nullptr;
    }
    auto* gfx = new g4f_gfx();
    gfx->window = window;

    DXGI_SWAP_CHAIN_DESC scd{};
    // BGRA backbuffer improves Direct2D interop for UI overlay renderers.
    scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.SampleDesc.Count = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 2;
    scd.OutputWindow = window->state.hwnd;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;
    flags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;
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
        g4f_set_last_hresult_error("g4f_gfx_create: D3D11CreateDeviceAndSwapChain failed", hr);
        g4f_gfx_destroy(gfx);
        return nullptr;
    }

    if (!gfxEnsureSize(gfx)) {
        g4f_set_last_error("g4f_gfx_create: swapchain resize/targets failed");
        g4f_gfx_destroy(gfx);
        return nullptr;
    }

    if (!gfxCreateShadersAndCube(gfx)) {
        g4f_set_last_error("g4f_gfx_create: shader/bootstrap resources failed");
        g4f_gfx_destroy(gfx);
        return nullptr;
    }

    if (!gfxCreateDefaultStates(gfx)) {
        g4f_set_last_error("g4f_gfx_create: default state creation failed");
        g4f_gfx_destroy(gfx);
        return nullptr;
    }

    if (!gfxCreateUnlitPipeline(gfx)) {
        g4f_set_last_error("g4f_gfx_create: unlit pipeline creation failed");
        g4f_gfx_destroy(gfx);
        return nullptr;
    }

    return gfx;
}

void g4f_gfx_destroy(g4f_gfx* gfx) {
    if (!gfx) return;
    safeRelease((IUnknown**)&gfx->sampLinearClamp);
    safeRelease((IUnknown**)&gfx->cbUnlit);
    safeRelease((IUnknown**)&gfx->ilUnlit);
    safeRelease((IUnknown**)&gfx->psLit);
    safeRelease((IUnknown**)&gfx->psUnlit);
    safeRelease((IUnknown**)&gfx->vsUnlit);
    safeRelease((IUnknown**)&gfx->cbMvp);
    safeRelease((IUnknown**)&gfx->ib);
    safeRelease((IUnknown**)&gfx->vb);
    safeRelease((IUnknown**)&gfx->inputLayout);
    safeRelease((IUnknown**)&gfx->ps);
    safeRelease((IUnknown**)&gfx->vs);
    safeRelease((IUnknown**)&gfx->bsOpaque);
    safeRelease((IUnknown**)&gfx->bsAlpha);
    safeRelease((IUnknown**)&gfx->dsDepthLess);
    safeRelease((IUnknown**)&gfx->dsDepthLessNoWrite);
    safeRelease((IUnknown**)&gfx->dsDisabled);
    safeRelease((IUnknown**)&gfx->rsCullBack);
    safeRelease((IUnknown**)&gfx->rsCullNone);
    safeRelease((IUnknown**)&gfx->rsCullFront);
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

    gfx->ctx->RSSetState(gfx->rsCullBack);
    gfx->ctx->OMSetDepthStencilState(gfx->dsDepthLess, 0);
    float blendFactor[4] = {0, 0, 0, 0};
    gfx->ctx->OMSetBlendState(gfx->bsOpaque, blendFactor, 0xFFFFFFFFu);

    // Reset state cache each frame (simple + robust).
    gfx->cachePipeline = 0;
    gfx->cacheIL = nullptr;
    gfx->cacheVS = nullptr;
    gfx->cachePS = nullptr;
    gfx->cacheVB = nullptr;
    gfx->cacheIB = nullptr;
    gfx->cacheVBStride = 0;
    gfx->cacheVBOffset = 0;
    gfx->cacheTopo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    gfx->cacheBlend = gfx->bsOpaque;
    gfx->cacheDepth = gfx->dsDepthLess;
    gfx->cacheRS = gfx->rsCullBack;
    gfx->cacheSRV0 = nullptr;
    gfx->cacheSamp0 = nullptr;
    gfx->cacheCB0VS = nullptr;
    gfx->cacheCB0PS = nullptr;

    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = (FLOAT)gfx->cachedW;
    vp.Height = (FLOAT)gfx->cachedH;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    gfx->ctx->RSSetViewports(1, &vp);
}

void g4f_gfx_get_size(const g4f_gfx* gfx, int* width, int* height) {
    if (!gfx) return;
    if (width) *width = gfx->cachedW;
    if (height) *height = gfx->cachedH;
}

float g4f_gfx_aspect(const g4f_gfx* gfx) {
    if (!gfx) return 1.0f;
    if (gfx->cachedH <= 0) return 1.0f;
    return (float)gfx->cachedW / (float)gfx->cachedH;
}

void g4f_gfx_set_vsync(g4f_gfx* gfx, int enabled) {
    if (!gfx) return;
    gfx->vsync = enabled ? 1 : 0;
}

void g4f_gfx_set_light_dir(g4f_gfx* gfx, float x, float y, float z) {
    if (!gfx) return;
    gfx->lightDir[0] = x;
    gfx->lightDir[1] = y;
    gfx->lightDir[2] = z;
    float v[3] = {gfx->lightDir[0], gfx->lightDir[1], gfx->lightDir[2]};
    vec3Normalize(v);
    gfx->lightDir[0] = v[0];
    gfx->lightDir[1] = v[1];
    gfx->lightDir[2] = v[2];
    gfx->lightDir[3] = 0.0f;
}

void g4f_gfx_set_light_colors(g4f_gfx* gfx, uint32_t lightRgba, uint32_t ambientRgba) {
    if (!gfx) return;
    rgbaU32ToFloat4(lightRgba, gfx->lightColor);
    rgbaU32ToFloat4(ambientRgba, gfx->ambientColor);
}

void g4f_gfx_draw_debug_cube(g4f_gfx* gfx, float timeSeconds) {
    if (!gfx || !gfx->ctx) return;

    if (gfx->cachePipeline != 1) {
        gfx->cachePipeline = 1;
        gfx->cacheIL = nullptr;
        gfx->cacheVS = nullptr;
        gfx->cachePS = nullptr;
        gfx->cacheVB = nullptr;
        gfx->cacheIB = nullptr;
        gfx->cacheTopo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        gfx->cacheCB0VS = nullptr;
        gfx->cacheCB0PS = nullptr;
        gfx->cacheSRV0 = nullptr;
        gfx->cacheSamp0 = nullptr;
    }

    if (gfx->cacheIL != gfx->inputLayout) {
        gfx->ctx->IASetInputLayout(gfx->inputLayout);
        gfx->cacheIL = gfx->inputLayout;
    }
    if (gfx->cacheVS != gfx->vs) {
        gfx->ctx->VSSetShader(gfx->vs, nullptr, 0);
        gfx->cacheVS = gfx->vs;
    }
    if (gfx->cachePS != gfx->ps) {
        gfx->ctx->PSSetShader(gfx->ps, nullptr, 0);
        gfx->cachePS = gfx->ps;
    }

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    if (gfx->cacheVB != gfx->vb || gfx->cacheVBStride != stride || gfx->cacheVBOffset != offset) {
        gfx->ctx->IASetVertexBuffers(0, 1, &gfx->vb, &stride, &offset);
        gfx->cacheVB = gfx->vb;
        gfx->cacheVBStride = stride;
        gfx->cacheVBOffset = offset;
    }
    if (gfx->cacheIB != gfx->ib) {
        gfx->ctx->IASetIndexBuffer(gfx->ib, DXGI_FORMAT_R16_UINT, 0);
        gfx->cacheIB = gfx->ib;
    }
    if (gfx->cacheTopo != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
        gfx->ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfx->cacheTopo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }

    float aspect = (gfx->cachedH > 0) ? ((float)gfx->cachedW / (float)gfx->cachedH) : 1.0f;
    g4f_mat4 proj = g4f_mat4_perspective(70.0f * 3.14159265f / 180.0f, aspect, 0.1f, 100.0f);
    g4f_mat4 view = g4f_mat4_translation(0.0f, 0.0f, 4.0f);
    g4f_mat4 rot = g4f_mat4_mul(g4f_mat4_rotation_y(timeSeconds * 0.8f), g4f_mat4_rotation_x(timeSeconds * 0.5f));
    g4f_mat4 mvp = g4f_mat4_mul(g4f_mat4_mul(rot, view), proj);

    gfx->ctx->UpdateSubresource(gfx->cbMvp, 0, nullptr, &mvp, 0, 0);
    if (gfx->cacheCB0VS != gfx->cbMvp) {
        gfx->ctx->VSSetConstantBuffers(0, 1, &gfx->cbMvp);
        gfx->cacheCB0VS = gfx->cbMvp;
    }

    gfx->ctx->DrawIndexed(gfx->indexCount, 0, 0);
}

struct g4f_gfx_texture {
    g4f_gfx* owner = nullptr;
    ID3D11Texture2D* tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    int width = 0;
    int height = 0;
    int dynamic = 0;
};

struct g4f_gfx_material {
    float tint[4]{1.0f, 1.0f, 1.0f, 1.0f};
    ID3D11ShaderResourceView* srv = nullptr; // optional
    int lit = 0;
    int alphaBlend = 0;
    int depthTest = 1;
    int depthWrite = 1;
    int cullMode = 0; // 0 back, 1 none, 2 front
};

struct g4f_gfx_mesh {
    ID3D11Buffer* vb = nullptr;
    ID3D11Buffer* ib = nullptr;
    UINT indexCount = 0;
};

g4f_gfx_texture* g4f_gfx_texture_create_rgba8(g4f_gfx* gfx, int width, int height, const void* rgbaPixels, int rowPitchBytes) {
    if (!gfx || !gfx->device || !rgbaPixels) { g4f_set_last_error("g4f_gfx_texture_create_rgba8: invalid args"); return nullptr; }
    if (width <= 0 || height <= 0) { g4f_set_last_error("g4f_gfx_texture_create_rgba8: invalid size"); return nullptr; }
    if (rowPitchBytes <= 0) rowPitchBytes = width * 4;

    auto* texture = new g4f_gfx_texture();
    texture->owner = gfx;
    texture->width = width;
    texture->height = height;
    texture->dynamic = 0;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = (UINT)width;
    desc.Height = (UINT)height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = rgbaPixels;
    data.SysMemPitch = (UINT)rowPitchBytes;

    HRESULT hr = gfx->device->CreateTexture2D(&desc, &data, &texture->tex);
    if (FAILED(hr) || !texture->tex) {
        g4f_set_last_hresult_error("g4f_gfx_texture_create_rgba8: CreateTexture2D failed", hr);
        g4f_gfx_texture_destroy(texture);
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = gfx->device->CreateShaderResourceView(texture->tex, &srvDesc, &texture->srv);
    if (FAILED(hr) || !texture->srv) {
        g4f_set_last_hresult_error("g4f_gfx_texture_create_rgba8: CreateShaderResourceView failed", hr);
        g4f_gfx_texture_destroy(texture);
        return nullptr;
    }

    return texture;
}

g4f_gfx_texture* g4f_gfx_texture_create_rgba8_dynamic(g4f_gfx* gfx, int width, int height) {
    if (!gfx || !gfx->device) { g4f_set_last_error("g4f_gfx_texture_create_rgba8_dynamic: invalid gfx"); return nullptr; }
    if (width <= 0 || height <= 0) { g4f_set_last_error("g4f_gfx_texture_create_rgba8_dynamic: invalid size"); return nullptr; }

    auto* texture = new g4f_gfx_texture();
    texture->owner = gfx;
    texture->width = width;
    texture->height = height;
    texture->dynamic = 1;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = (UINT)width;
    desc.Height = (UINT)height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = gfx->device->CreateTexture2D(&desc, nullptr, &texture->tex);
    if (FAILED(hr) || !texture->tex) {
        g4f_set_last_hresult_error("g4f_gfx_texture_create_rgba8_dynamic: CreateTexture2D failed", hr);
        g4f_gfx_texture_destroy(texture);
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = gfx->device->CreateShaderResourceView(texture->tex, &srvDesc, &texture->srv);
    if (FAILED(hr) || !texture->srv) {
        g4f_set_last_hresult_error("g4f_gfx_texture_create_rgba8_dynamic: CreateShaderResourceView failed", hr);
        g4f_gfx_texture_destroy(texture);
        return nullptr;
    }

    return texture;
}

int g4f_gfx_texture_update_rgba8(g4f_gfx_texture* texture, const void* rgbaPixels, int rowPitchBytes) {
    if (!texture || !texture->owner || !texture->owner->ctx) return 0;
    if (!texture->tex || !rgbaPixels) return 0;
    if (texture->width <= 0 || texture->height <= 0) return 0;
    if (rowPitchBytes <= 0) rowPitchBytes = texture->width * 4;

    if (texture->dynamic) {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        HRESULT hr = texture->owner->ctx->Map(texture->tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return 0;

        const uint8_t* src = (const uint8_t*)rgbaPixels;
        uint8_t* dst = (uint8_t*)mapped.pData;
        const int copyRowBytes = texture->width * 4;
        for (int y = 0; y < texture->height; y++) {
            std::memcpy(dst + y * (int)mapped.RowPitch, src + y * rowPitchBytes, (size_t)copyRowBytes);
        }

        texture->owner->ctx->Unmap(texture->tex, 0);
        return 1;
    }

    D3D11_BOX box{};
    box.left = 0;
    box.top = 0;
    box.front = 0;
    box.right = (UINT)texture->width;
    box.bottom = (UINT)texture->height;
    box.back = 1;
    texture->owner->ctx->UpdateSubresource(texture->tex, 0, &box, rgbaPixels, (UINT)rowPitchBytes, 0);
    return 1;
}

g4f_gfx_texture* g4f_gfx_texture_create_solid_rgba8(g4f_gfx* gfx, uint32_t rgba) {
    uint32_t pixel = rgba;
    return g4f_gfx_texture_create_rgba8(gfx, 1, 1, &pixel, 4);
}

g4f_gfx_texture* g4f_gfx_texture_create_checker_rgba8(g4f_gfx* gfx, int width, int height, int cellSizePx, uint32_t rgbaA, uint32_t rgbaB) {
    if (!gfx) return nullptr;
    if (width <= 0 || height <= 0) return nullptr;
    int cell = (cellSizePx <= 0) ? 8 : cellSizePx;
    std::vector<uint32_t> pixels((size_t)width * (size_t)height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int cx = (x / cell) & 1;
            int cy = (y / cell) & 1;
            int on = (cx ^ cy) & 1;
            pixels[(size_t)y * (size_t)width + (size_t)x] = on ? rgbaA : rgbaB;
        }
    }
    return g4f_gfx_texture_create_rgba8(gfx, width, height, pixels.data(), width * 4);
}

void g4f_gfx_texture_destroy(g4f_gfx_texture* texture) {
    if (!texture) return;
    safeRelease((IUnknown**)&texture->srv);
    safeRelease((IUnknown**)&texture->tex);
    delete texture;
}

void g4f_gfx_texture_get_size(const g4f_gfx_texture* texture, int* width, int* height) {
    if (!texture) return;
    if (width) *width = texture->width;
    if (height) *height = texture->height;
}

g4f_gfx_material* g4f_gfx_material_create_unlit(g4f_gfx* gfx, const g4f_gfx_material_unlit_desc* desc) {
    if (!gfx) { g4f_set_last_error("g4f_gfx_material_create_unlit: gfx is null"); return nullptr; }
    auto* material = new g4f_gfx_material();

    uint32_t rgba = desc ? desc->tintRgba : g4f_rgba_u32(255, 255, 255, 255);
    rgbaU32ToFloat4(rgba, material->tint);

    if (desc && desc->texture && desc->texture->srv) {
        material->srv = desc->texture->srv;
        material->srv->AddRef();
    }

    material->alphaBlend = (desc && desc->alphaBlend) ? 1 : 0;
    material->depthTest = (!desc || desc->depthTest) ? 1 : 0;
    material->depthWrite = (!desc || desc->depthWrite) ? 1 : 0;
    material->cullMode = desc ? desc->cullMode : 0;
    if (material->cullMode < 0) material->cullMode = 0;
    if (material->cullMode > 2) material->cullMode = 2;

    return material;
}

g4f_gfx_material* g4f_gfx_material_create_lit(g4f_gfx* gfx, const g4f_gfx_material_unlit_desc* desc) {
    g4f_gfx_material* m = g4f_gfx_material_create_unlit(gfx, desc);
    if (m) m->lit = 1;
    return m;
}

void g4f_gfx_material_destroy(g4f_gfx_material* material) {
    if (!material) return;
    safeRelease((IUnknown**)&material->srv);
    delete material;
}

void g4f_gfx_material_set_tint_rgba(g4f_gfx_material* material, uint32_t rgba) {
    if (!material) return;
    rgbaU32ToFloat4(rgba, material->tint);
}

void g4f_gfx_material_set_texture(g4f_gfx_material* material, g4f_gfx_texture* texture) {
    if (!material) return;
    safeRelease((IUnknown**)&material->srv);
    if (texture && texture->srv) {
        material->srv = texture->srv;
        material->srv->AddRef();
    }
}

void g4f_gfx_material_set_alpha_blend(g4f_gfx_material* material, int enabled) {
    if (!material) return;
    material->alphaBlend = enabled ? 1 : 0;
}

void g4f_gfx_material_set_depth(g4f_gfx_material* material, int depthTest, int depthWrite) {
    if (!material) return;
    material->depthTest = depthTest ? 1 : 0;
    material->depthWrite = depthWrite ? 1 : 0;
}

void g4f_gfx_material_set_cull(g4f_gfx_material* material, int cullMode) {
    if (!material) return;
    int cm = cullMode;
    if (cm < 0) cm = 0;
    if (cm > 2) cm = 2;
    material->cullMode = cm;
}

g4f_gfx_mesh* g4f_gfx_mesh_create_p3n3uv2(g4f_gfx* gfx, const g4f_gfx_vertex_p3n3uv2* vertices, int vertexCount, const uint16_t* indices, int indexCount) {
    if (!gfx || !gfx->device) { g4f_set_last_error("g4f_gfx_mesh_create_p3n3uv2: invalid gfx"); return nullptr; }
    if (!vertices || vertexCount <= 0) { g4f_set_last_error("g4f_gfx_mesh_create_p3n3uv2: invalid vertices"); return nullptr; }
    if (!indices || indexCount <= 0) { g4f_set_last_error("g4f_gfx_mesh_create_p3n3uv2: invalid indices"); return nullptr; }

    auto* mesh = new g4f_gfx_mesh();
    mesh->indexCount = (UINT)indexCount;

    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.ByteWidth = (UINT)(sizeof(g4f_gfx_vertex_p3n3uv2) * (size_t)vertexCount);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData{};
    vbData.pSysMem = vertices;
    HRESULT hr = gfx->device->CreateBuffer(&vbDesc, &vbData, &mesh->vb);
    if (FAILED(hr) || !mesh->vb) {
        g4f_set_last_hresult_error("g4f_gfx_mesh_create_p3n3uv2: CreateBuffer(vb) failed", hr);
        g4f_gfx_mesh_destroy(mesh);
        return nullptr;
    }

    D3D11_BUFFER_DESC ibDesc{};
    ibDesc.ByteWidth = (UINT)(sizeof(uint16_t) * (size_t)indexCount);
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibData{};
    ibData.pSysMem = indices;
    hr = gfx->device->CreateBuffer(&ibDesc, &ibData, &mesh->ib);
    if (FAILED(hr) || !mesh->ib) {
        g4f_set_last_hresult_error("g4f_gfx_mesh_create_p3n3uv2: CreateBuffer(ib) failed", hr);
        g4f_gfx_mesh_destroy(mesh);
        return nullptr;
    }

    return mesh;
}

g4f_gfx_mesh* g4f_gfx_mesh_create_cube_p3n3uv2(g4f_gfx* gfx, float halfExtent) {
    float e = (halfExtent > 0.0f) ? halfExtent : 1.0f;
    const g4f_gfx_vertex_p3n3uv2 vertices[] = {
        // -Z
        {-e,-e,-e, 0,0,-1, 0,1}, {+e,-e,-e, 0,0,-1, 1,1}, {+e,+e,-e, 0,0,-1, 1,0}, {-e,+e,-e, 0,0,-1, 0,0},
        // +Z
        {-e,-e,+e, 0,0,+1, 0,1}, {-e,+e,+e, 0,0,+1, 0,0}, {+e,+e,+e, 0,0,+1, 1,0}, {+e,-e,+e, 0,0,+1, 1,1},
        // -Y
        {-e,-e,-e, 0,-1,0, 0,1}, {-e,-e,+e, 0,-1,0, 0,0}, {+e,-e,+e, 0,-1,0, 1,0}, {+e,-e,-e, 0,-1,0, 1,1},
        // +Y
        {-e,+e,-e, 0,+1,0, 0,1}, {+e,+e,-e, 0,+1,0, 1,1}, {+e,+e,+e, 0,+1,0, 1,0}, {-e,+e,+e, 0,+1,0, 0,0},
        // +X
        {+e,-e,-e, +1,0,0, 0,1}, {+e,-e,+e, +1,0,0, 1,1}, {+e,+e,+e, +1,0,0, 1,0}, {+e,+e,-e, +1,0,0, 0,0},
        // -X
        {-e,-e,-e, -1,0,0, 1,1}, {-e,+e,-e, -1,0,0, 1,0}, {-e,+e,+e, -1,0,0, 0,0}, {-e,-e,+e, -1,0,0, 0,1},
    };
    const uint16_t indices[] = {
        0,1,2, 0,2,3,
        4,5,6, 4,6,7,
        8,9,10, 8,10,11,
        12,13,14, 12,14,15,
        16,17,18, 16,18,19,
        20,21,22, 20,22,23,
    };
    return g4f_gfx_mesh_create_p3n3uv2(
        gfx,
        vertices,
        (int)(sizeof(vertices) / sizeof(vertices[0])),
        indices,
        (int)(sizeof(indices) / sizeof(indices[0]))
    );
}

g4f_gfx_mesh* g4f_gfx_mesh_create_plane_xz_p3n3uv2(g4f_gfx* gfx, float halfExtent, float uvScale) {
    float e = (halfExtent > 0.0f) ? halfExtent : 1.0f;
    float s = (uvScale > 0.0f) ? uvScale : 1.0f;
    const g4f_gfx_vertex_p3n3uv2 vertices[] = {
        {-e, 0.0f, -e, 0.0f, 1.0f, 0.0f, 0.0f, s},
        {+e, 0.0f, -e, 0.0f, 1.0f, 0.0f, s, s},
        {+e, 0.0f, +e, 0.0f, 1.0f, 0.0f, s, 0.0f},
        {-e, 0.0f, +e, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
    };
    const uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    return g4f_gfx_mesh_create_p3n3uv2(gfx, vertices, 4, indices, 6);
}

void g4f_gfx_mesh_destroy(g4f_gfx_mesh* mesh) {
    if (!mesh) return;
    safeRelease((IUnknown**)&mesh->ib);
    safeRelease((IUnknown**)&mesh->vb);
    delete mesh;
}

void g4f_gfx_draw_mesh(g4f_gfx* gfx, const g4f_gfx_mesh* mesh, const g4f_gfx_material* material, const g4f_mat4* mvp) {
    g4f_gfx_draw_mesh_xform(gfx, mesh, material, nullptr, mvp);
}

void g4f_gfx_draw_mesh_xform(g4f_gfx* gfx, const g4f_gfx_mesh* mesh, const g4f_gfx_material* material, const g4f_mat4* model, const g4f_mat4* mvp) {
    if (!gfx || !gfx->ctx) return;
    if (!mesh || !mesh->vb || !mesh->ib) return;
    if (!material || !mvp) return;

    if (gfx->cachePipeline != 2) {
        gfx->cachePipeline = 2;
        gfx->cacheIL = nullptr;
        gfx->cacheVS = nullptr;
        gfx->cachePS = nullptr;
        gfx->cacheVB = nullptr;
        gfx->cacheIB = nullptr;
        gfx->cacheTopo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        gfx->cacheCB0VS = nullptr;
        gfx->cacheCB0PS = nullptr;
        gfx->cacheSRV0 = nullptr;
        gfx->cacheSamp0 = nullptr;
    }

    if (gfx->cacheIL != gfx->ilUnlit) {
        gfx->ctx->IASetInputLayout(gfx->ilUnlit);
        gfx->cacheIL = gfx->ilUnlit;
    }
    if (gfx->cacheVS != gfx->vsUnlit) {
        gfx->ctx->VSSetShader(gfx->vsUnlit, nullptr, 0);
        gfx->cacheVS = gfx->vsUnlit;
    }
    ID3D11PixelShader* desiredPS = material->lit ? gfx->psLit : gfx->psUnlit;
    if (gfx->cachePS != desiredPS) {
        gfx->ctx->PSSetShader(desiredPS, nullptr, 0);
        gfx->cachePS = desiredPS;
    }

    UINT stride = (UINT)sizeof(g4f_gfx_vertex_p3n3uv2);
    UINT offset = 0;
    if (gfx->cacheVB != mesh->vb || gfx->cacheVBStride != stride || gfx->cacheVBOffset != offset) {
        gfx->ctx->IASetVertexBuffers(0, 1, &mesh->vb, &stride, &offset);
        gfx->cacheVB = mesh->vb;
        gfx->cacheVBStride = stride;
        gfx->cacheVBOffset = offset;
    }
    if (gfx->cacheIB != mesh->ib) {
        gfx->ctx->IASetIndexBuffer(mesh->ib, DXGI_FORMAT_R16_UINT, 0);
        gfx->cacheIB = mesh->ib;
    }
    if (gfx->cacheTopo != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
        gfx->ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfx->cacheTopo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }

    // Per-material pipeline state.
    float blendFactor[4] = {0, 0, 0, 0};
    ID3D11BlendState* desiredBlend = material->alphaBlend ? gfx->bsAlpha : gfx->bsOpaque;
    if (gfx->cacheBlend != desiredBlend) {
        gfx->ctx->OMSetBlendState(desiredBlend, blendFactor, 0xFFFFFFFFu);
        gfx->cacheBlend = desiredBlend;
    }
    ID3D11DepthStencilState* desiredDepth = nullptr;
    if (!material->depthTest) desiredDepth = gfx->dsDisabled;
    else desiredDepth = material->depthWrite ? gfx->dsDepthLess : gfx->dsDepthLessNoWrite;
    if (gfx->cacheDepth != desiredDepth) {
        gfx->ctx->OMSetDepthStencilState(desiredDepth, 0);
        gfx->cacheDepth = desiredDepth;
    }
    ID3D11RasterizerState* rs = gfx->rsCullBack;
    if (material->cullMode == 1) rs = gfx->rsCullNone;
    if (material->cullMode == 2) rs = gfx->rsCullFront;
    if (gfx->cacheRS != rs) {
        gfx->ctx->RSSetState(rs);
        gfx->cacheRS = rs;
    }

    CbMaterial cb{};
    cb.mvp = *mvp;
    cb.tint[0] = material->tint[0];
    cb.tint[1] = material->tint[1];
    cb.tint[2] = material->tint[2];
    cb.tint[3] = material->tint[3];
    cb.hasTex = material->srv ? 1.0f : 0.0f;
    cb.model = model ? *model : g4f_mat4_identity();
    cb.normal = mat4NormalMatrix(cb.model);
    cb.lightDir[0] = gfx->lightDir[0];
    cb.lightDir[1] = gfx->lightDir[1];
    cb.lightDir[2] = gfx->lightDir[2];
    cb.lightDir[3] = 0.0f;
    cb.lightColor[0] = gfx->lightColor[0];
    cb.lightColor[1] = gfx->lightColor[1];
    cb.lightColor[2] = gfx->lightColor[2];
    cb.lightColor[3] = gfx->lightColor[3];
    cb.ambientColor[0] = gfx->ambientColor[0];
    cb.ambientColor[1] = gfx->ambientColor[1];
    cb.ambientColor[2] = gfx->ambientColor[2];
    cb.ambientColor[3] = gfx->ambientColor[3];
    gfx->ctx->UpdateSubresource(gfx->cbUnlit, 0, nullptr, &cb, 0, 0);
    if (gfx->cacheCB0VS != gfx->cbUnlit) {
        gfx->ctx->VSSetConstantBuffers(0, 1, &gfx->cbUnlit);
        gfx->cacheCB0VS = gfx->cbUnlit;
    }
    if (gfx->cacheCB0PS != gfx->cbUnlit) {
        gfx->ctx->PSSetConstantBuffers(0, 1, &gfx->cbUnlit);
        gfx->cacheCB0PS = gfx->cbUnlit;
    }

    ID3D11ShaderResourceView* srv = material->srv;
    if (gfx->cacheSRV0 != srv) {
        gfx->ctx->PSSetShaderResources(0, 1, &srv);
        gfx->cacheSRV0 = srv;
    }
    if (gfx->cacheSamp0 != gfx->sampLinearClamp) {
        gfx->ctx->PSSetSamplers(0, 1, &gfx->sampLinearClamp);
        gfx->cacheSamp0 = gfx->sampLinearClamp;
    }

    gfx->ctx->DrawIndexed(mesh->indexCount, 0, 0);
}

void g4f_gfx_end(g4f_gfx* gfx) {
    if (!gfx || !gfx->swapChain) return;
    gfx->swapChain->Present(gfx->vsync ? 1u : 0u, 0);
}
