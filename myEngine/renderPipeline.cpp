#include"renderPipeline.h"
#include"Ssao.h"
std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};
std::vector<D3D12_INPUT_ELEMENT_DESC> mSkinnedInputLayout =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

};

myPso::myPso(ComPtr<ID3D12Device> md3dDevice,
    psoStyle pipelineStyle,
    ComPtr<ID3DBlob> mvsByteCode,
    ComPtr<ID3DBlob> mpsByteCode,
    ComPtr<ID3DBlob> mhsByteCode,
    ComPtr<ID3DBlob> mdsByteCode,
    bool m4xMsaaState,
    UINT m4xMsaaQuality)
{
    
    switch (pipelineStyle)
    {
    case default_pso:
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
        myRootSignature temp(md3dDevice, rootSignatureStyle::default_RSS);
        ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
        opaquePsoDesc.pRootSignature = temp.GetRootSignature().Get();
        opaquePsoDesc.VS =
        {
            reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
            mvsByteCode->GetBufferSize()
        };
        opaquePsoDesc.PS =
        {
            reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
            mpsByteCode->GetBufferSize()
        };
        opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        opaquePsoDesc.SampleMask = UINT_MAX;
        opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        opaquePsoDesc.NumRenderTargets = 1;
        opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
        opaquePsoDesc.SampleDesc.Count = 1;
        opaquePsoDesc.SampleDesc.Quality =  0;
        opaquePsoDesc.DSVFormat = mDepthStencilFormat;
        ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSO)));

    }
    break;
    case skinned_pso:
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
        myRootSignature temp(md3dDevice, rootSignatureStyle::default_RSS);
        ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        opaquePsoDesc.InputLayout = { mSkinnedInputLayout.data(), (UINT)mSkinnedInputLayout.size() };
        opaquePsoDesc.pRootSignature = temp.GetRootSignature().Get();
        opaquePsoDesc.VS =
        {
            reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
            mvsByteCode->GetBufferSize()
        };
        opaquePsoDesc.PS =
        {
            reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
            mpsByteCode->GetBufferSize()
        };
        opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        opaquePsoDesc.SampleMask = UINT_MAX;
        opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        opaquePsoDesc.NumRenderTargets = 1;
        opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
        opaquePsoDesc.SampleDesc.Count =  1;
        opaquePsoDesc.SampleDesc.Quality =  0;
        opaquePsoDesc.DSVFormat = mDepthStencilFormat;
        ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSO)));

    }
    break;
    case shadow_map:
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
        myRootSignature temp(md3dDevice, rootSignatureStyle::default_RSS);
        ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
        opaquePsoDesc.pRootSignature = temp.GetRootSignature().Get();
        opaquePsoDesc.VS =
        {
            reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
            mvsByteCode->GetBufferSize()
        };
        opaquePsoDesc.PS =
        {
            reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
            mpsByteCode->GetBufferSize()
        };
        opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        opaquePsoDesc.SampleMask = UINT_MAX;
        opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        opaquePsoDesc.NumRenderTargets = 0;
        opaquePsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
        opaquePsoDesc.SampleDesc.Count = 1;
        opaquePsoDesc.SampleDesc.Quality =  0;
        opaquePsoDesc.DSVFormat = mDepthStencilFormat;
        opaquePsoDesc.RasterizerState.DepthBias = 100000;
        opaquePsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        opaquePsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
        ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSO)));

       }
    break;
    case skin_shadow:
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
        myRootSignature temp(md3dDevice, rootSignatureStyle::default_RSS);
        ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        opaquePsoDesc.InputLayout = { mSkinnedInputLayout.data(), (UINT)mSkinnedInputLayout.size() };
        opaquePsoDesc.pRootSignature = temp.GetRootSignature().Get();
        opaquePsoDesc.VS =
        {
            reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
            mvsByteCode->GetBufferSize()
        };
        opaquePsoDesc.PS =
        {
            reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
            mpsByteCode->GetBufferSize()
        };
        opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        opaquePsoDesc.SampleMask = UINT_MAX;
        opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        opaquePsoDesc.NumRenderTargets = 0;
        opaquePsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
        opaquePsoDesc.SampleDesc.Count = 1;
        opaquePsoDesc.SampleDesc.Quality =  0;
        opaquePsoDesc.DSVFormat = mDepthStencilFormat;
        opaquePsoDesc.RasterizerState.DepthBias = 100000;
        opaquePsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        opaquePsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
        ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSO)));    
    }
        break;
    case drawing_normal:
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
        myRootSignature temp(md3dDevice, rootSignatureStyle::default_RSS);
        ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
        opaquePsoDesc.pRootSignature = temp.GetRootSignature().Get();
        opaquePsoDesc.VS =
        {
            reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
            mvsByteCode->GetBufferSize()
        };
        opaquePsoDesc.PS =
        {
            reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
            mpsByteCode->GetBufferSize()
        };
        opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        opaquePsoDesc.SampleMask = UINT_MAX;
        opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        opaquePsoDesc.NumRenderTargets = 1;
        opaquePsoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        opaquePsoDesc.SampleDesc.Count = 1;
        opaquePsoDesc.SampleDesc.Quality =  0;
        opaquePsoDesc.DSVFormat = mDepthStencilFormat;
        ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSO)));       
    }
        break;
    case skin_drawing_normal:
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
        myRootSignature temp(md3dDevice, rootSignatureStyle::default_RSS);
        ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        opaquePsoDesc.InputLayout = { mSkinnedInputLayout.data(), (UINT)mSkinnedInputLayout.size() };
        opaquePsoDesc.pRootSignature = temp.GetRootSignature().Get();
        opaquePsoDesc.VS =
        {
            reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
            mvsByteCode->GetBufferSize()
        };
        opaquePsoDesc.PS =
        {
            reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
            mpsByteCode->GetBufferSize()
        };
        opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        opaquePsoDesc.SampleMask = UINT_MAX;
        opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        opaquePsoDesc.NumRenderTargets = 1;
        opaquePsoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
        opaquePsoDesc.SampleDesc.Count = 1;
        opaquePsoDesc.SampleDesc.Quality = 0;
        opaquePsoDesc.DSVFormat = mDepthStencilFormat;
        ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSO)));

    }
        break;
    case ssao_pso:
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
        myRootSignature temp(md3dDevice, rootSignatureStyle::SSAO_RSS);
        ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        opaquePsoDesc.InputLayout = { nullptr,0 };
        opaquePsoDesc.pRootSignature = temp.GetRootSignature().Get();
        opaquePsoDesc.VS =
        {
            reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
            mvsByteCode->GetBufferSize()
        };
        opaquePsoDesc.PS =
        {
            reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
            mpsByteCode->GetBufferSize()
        };
        opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        opaquePsoDesc.DepthStencilState.DepthEnable = false;
        opaquePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        opaquePsoDesc.SampleMask = UINT_MAX;
        opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        opaquePsoDesc.NumRenderTargets = 1;
        opaquePsoDesc.RTVFormats[0] = Ssao::AmbientMapFormat;
        opaquePsoDesc.SampleDesc.Count =  1;
        opaquePsoDesc.SampleDesc.Quality =  0;
        opaquePsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSO)));

    }
    break;
    case sky_pso:
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
        myRootSignature temp(md3dDevice, rootSignatureStyle::default_RSS);
        ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
        opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
        opaquePsoDesc.pRootSignature = temp.GetRootSignature().Get();
        opaquePsoDesc.VS =
        {
            reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
            mvsByteCode->GetBufferSize()
        };
        opaquePsoDesc.PS =
        {
            reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
            mpsByteCode->GetBufferSize()
        };
        opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        opaquePsoDesc.RasterizerState.CullMode= D3D12_CULL_MODE_NONE;
        opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        opaquePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        opaquePsoDesc.SampleMask = UINT_MAX;
        opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        opaquePsoDesc.NumRenderTargets = 1;
        opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
        opaquePsoDesc.SampleDesc.Count =  1;
        opaquePsoDesc.SampleDesc.Quality =  0;
        opaquePsoDesc.DSVFormat = mDepthStencilFormat;
        ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSO)));

    }
        break;
    default:
    {
            }
    break;
    }
}

ComPtr<ID3D12PipelineState> myPso::GetPipeline()
{
    return mPSO;
}