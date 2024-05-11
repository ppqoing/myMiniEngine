#pragma once
#include"Common/d3dUtil.h"
#include"Sigleton.h"
#include<queue>
#define SRV_HEAP_MAX_SIZE 1024;
#define Rtv_HEAP_MAX_SIZE 16;
#define Dsv_HEAP_MAX_SIZE 4;
class DescriptorManager:public Sigleton<DescriptorManager>
{
	friend class Sigleton<DescriptorManager>;
public:
	void init(ComPtr<ID3D12Device> Device);
	UINT CreateSRV_CBV_UAV( ComPtr<ID3D12Resource> resource, D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc);
	UINT CreateRtv(ComPtr<ID3D12Resource> resource, D3D12_RENDER_TARGET_VIEW_DESC viewDesc);
	UINT CreateDsv(ComPtr<ID3D12Resource> resource, D3D12_DEPTH_STENCIL_VIEW_DESC  viewDesc);

	ComPtr<ID3D12DescriptorHeap> GetSRVDescriptorHeap();
	ComPtr<ID3D12DescriptorHeap> GetRTVDescriptorHeap();
	ComPtr<ID3D12DescriptorHeap> GetDSVDescriptorHeap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVDescriptorHandle_CPU(UINT index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVDescriptorHandle_GPU(UINT index);
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVDescriptorHandle_CPU(UINT index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetRTVDescriptorHandle_GPU(UINT index);
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVDescriptorHandle_CPU(UINT index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetDSVDescriptorHandle_GPU(UINT index);

private:
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap=nullptr;
	ComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDsvDescriptorHeap = nullptr;

	queue<UINT> SRVIndexQueue;
	queue<UINT> RtvIndexQueue;
	queue<UINT> DsvIndexQueue;

	UINT mCbvSrvUavDescriptorSize;
	UINT mRtvDescriptorSize;
	UINT mDsvDescriptorSize;

	ComPtr<ID3D12Device> md3dDevice;

	D3D12_GPU_DESCRIPTOR_HANDLE SRV_Start_Handle_GPU;
	D3D12_CPU_DESCRIPTOR_HANDLE SRV_Start_Handle_CPU;

	D3D12_GPU_DESCRIPTOR_HANDLE RTV_Start_Handle_GPU;
	D3D12_CPU_DESCRIPTOR_HANDLE RTV_Start_Handle_CPU;

	D3D12_GPU_DESCRIPTOR_HANDLE DSV_Start_Handle_GPU;
	D3D12_CPU_DESCRIPTOR_HANDLE DSV_Start_Handle_CPU;

};
