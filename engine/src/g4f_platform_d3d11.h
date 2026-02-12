#pragma once

#include "g4f_platform_win32.h"

#include <d3d11.h>
#include <dxgi.h>
#include <cstdint>

struct g4f_gfx {
    g4f_window* window = nullptr;
    int cachedW = 0;
    int cachedH = 0;
    int vsync = 1;
    uint64_t backbufferGeneration = 0;

    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11Texture2D* depthTex = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;

    ID3D11RasterizerState* rsCullBack = nullptr;
    ID3D11RasterizerState* rsCullNone = nullptr;
    ID3D11RasterizerState* rsCullFront = nullptr;
    ID3D11DepthStencilState* dsDepthLess = nullptr;
    ID3D11DepthStencilState* dsDepthLessNoWrite = nullptr;
    ID3D11DepthStencilState* dsDisabled = nullptr;
    ID3D11BlendState* bsOpaque = nullptr;
    ID3D11BlendState* bsAlpha = nullptr;

    float lightDir[4] = {0.0f, -1.0f, 0.0f, 0.0f};
    float lightColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float ambientColor[4] = {0.15f, 0.15f, 0.18f, 1.0f};

    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;

    ID3D11Buffer* vb = nullptr;
    ID3D11Buffer* ib = nullptr;
    ID3D11Buffer* cbMvp = nullptr;

    // Unlit material pipeline (P3N3UV2, optional texture).
    ID3D11VertexShader* vsUnlit = nullptr;
    ID3D11PixelShader* psUnlit = nullptr;
    ID3D11PixelShader* psLit = nullptr;
    ID3D11InputLayout* ilUnlit = nullptr;
    ID3D11Buffer* cbUnlit = nullptr;
    ID3D11SamplerState* sampLinearClamp = nullptr;

    UINT indexCount = 0;

    // Lightweight state cache (avoid redundant Set* calls in hot draw paths).
    int cachePipeline = 0; // 0 none, 1 debug, 2 mesh
    ID3D11InputLayout* cacheIL = nullptr;
    ID3D11VertexShader* cacheVS = nullptr;
    ID3D11PixelShader* cachePS = nullptr;
    ID3D11Buffer* cacheVB = nullptr;
    ID3D11Buffer* cacheIB = nullptr;
    UINT cacheVBStride = 0;
    UINT cacheVBOffset = 0;
    D3D11_PRIMITIVE_TOPOLOGY cacheTopo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11BlendState* cacheBlend = nullptr;
    ID3D11DepthStencilState* cacheDepth = nullptr;
    ID3D11RasterizerState* cacheRS = nullptr;
    ID3D11ShaderResourceView* cacheSRV0 = nullptr;
    ID3D11SamplerState* cacheSamp0 = nullptr;
    ID3D11Buffer* cacheCB0VS = nullptr;
    ID3D11Buffer* cacheCB0PS = nullptr;
};
