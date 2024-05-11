#pragma once
#include"rootSignature.h"
class myPso
{

public:
	myPso(ComPtr<ID3D12Device> md3dDevice,
		psoStyle pipelineStyle = psoStyle::default_pso,
		ComPtr<ID3DBlob> mvsByteCode = nullptr,
		ComPtr<ID3DBlob> mpsByteCode = nullptr,
		ComPtr<ID3DBlob> mhsByteCode = nullptr,
		ComPtr<ID3DBlob> mdsByteCode = nullptr,
		bool m4xMsaaState = true,
		UINT m4xMsaaQuality = 0);
	ComPtr<ID3D12PipelineState> GetPipeline();
private:
	ComPtr<ID3D12PipelineState> mPSO=nullptr;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};