#pragma once
#include"Common/d3dUtil.h"
enum rootSignatureStyle
{
	default_RSS,
	SSAO_RSS
};
class myRootSignature
{
public:
	myRootSignature(ComPtr<ID3D12Device> md3dDevice, rootSignatureStyle style = rootSignatureStyle::default_RSS);
	ComPtr<ID3D12RootSignature> GetRootSignature();
private:
	ComPtr<ID3D12RootSignature> mRootSignature;
};
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();