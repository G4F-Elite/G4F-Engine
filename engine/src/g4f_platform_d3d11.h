#pragma once

#include "g4f_platform_win32.h"

#include <d3d11.h>
#include <dxgi.h>

struct g4f_gfx {
    g4f_window* window = nullptr;
    int cachedW = 0;
    int cachedH = 0;

    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11Texture2D* depthTex = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;

    ID3D11RasterizerState* rsCullBack = nullptr;
    ID3D11DepthStencilState* dsDepthLess = nullptr;
    ID3D11BlendState* bsOpaque = nullptr;

    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;

    ID3D11Buffer* vb = nullptr;
    ID3D11Buffer* ib = nullptr;
    ID3D11Buffer* cbMvp = nullptr;

    // Unlit material pipeline (P3N3UV2, optional texture).
    ID3D11VertexShader* vsUnlit = nullptr;
    ID3D11PixelShader* psUnlit = nullptr;
    ID3D11InputLayout* ilUnlit = nullptr;
    ID3D11Buffer* cbUnlit = nullptr;
    ID3D11SamplerState* sampLinearClamp = nullptr;

    UINT indexCount = 0;
};
