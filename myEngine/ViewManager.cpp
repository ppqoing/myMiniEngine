#include"ViewManager.h"

void DescriptorManager::init(ComPtr<ID3D12Device> Device)
{
	md3dDevice = Device;

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = SRV_HEAP_MAX_SIZE;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = Rtv_HEAP_MAX_SIZE;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvDescriptorHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = Dsv_HEAP_MAX_SIZE;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvDescriptorHeap.GetAddressOf())));

	size_t maxSize = SRV_HEAP_MAX_SIZE;
	for (size_t i = 0; i < maxSize; i++)
	{
		SRVIndexQueue.push(i);
	}
	maxSize = Rtv_HEAP_MAX_SIZE;
	for (size_t i = 0; i < maxSize; i++)
	{
		RtvIndexQueue.push(i);
	}
	maxSize = Dsv_HEAP_MAX_SIZE;
	for (size_t i = 0; i < maxSize; i++)
	{
		DsvIndexQueue.push(i);
	}
	
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	SRV_Start_Handle_GPU = mSrvDescriptorHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	SRV_Start_Handle_CPU = mSrvDescriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	RTV_Start_Handle_GPU = mRtvDescriptorHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	RTV_Start_Handle_CPU = mRtvDescriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart();
	DSV_Start_Handle_GPU = mDsvDescriptorHeap.Get()->GetGPUDescriptorHandleForHeapStart();
	DSV_Start_Handle_CPU = mDsvDescriptorHeap.Get()->GetCPUDescriptorHandleForHeapStart();
}

UINT DescriptorManager::CreateSRV_CBV_UAV( ComPtr<ID3D12Resource> resource, D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc)
{
	UINT index_now =SRVIndexQueue.front();
	auto handle_srv = mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle_srv.ptr += mCbvSrvUavDescriptorSize * index_now;
	md3dDevice->CreateShaderResourceView(resource.Get(), &viewDesc, handle_srv);
	SRVIndexQueue.pop();
	return index_now;
}

UINT DescriptorManager::CreateRtv(ComPtr<ID3D12Resource> resource, D3D12_RENDER_TARGET_VIEW_DESC viewDesc)
{
	UINT index_now = RtvIndexQueue.front();
	auto handle = mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += mRtvDescriptorSize * index_now;
	md3dDevice->CreateRenderTargetView(resource.Get(), &viewDesc, handle);
	RtvIndexQueue.pop();
	return index_now;
}

UINT DescriptorManager::CreateDsv(ComPtr<ID3D12Resource> resource, D3D12_DEPTH_STENCIL_VIEW_DESC  viewDesc)
{
	UINT index_now = DsvIndexQueue.front();
	auto handle = mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += mDsvDescriptorSize * index_now;
	md3dDevice->CreateDepthStencilView(resource.Get(), &viewDesc, handle);
	DsvIndexQueue.pop();
	return index_now;
}

ComPtr<ID3D12DescriptorHeap> DescriptorManager::GetSRVDescriptorHeap()
{
	return mSrvDescriptorHeap;
}

ComPtr<ID3D12DescriptorHeap> DescriptorManager::GetRTVDescriptorHeap()
{
	return mRtvDescriptorHeap;
}

ComPtr<ID3D12DescriptorHeap> DescriptorManager::GetDSVDescriptorHeap()
{
	return mDsvDescriptorHeap;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetSRVDescriptorHandle_CPU(UINT index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE res;
	res.ptr= SRV_Start_Handle_CPU.ptr + index * mCbvSrvUavDescriptorSize;
	return res;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetSRVDescriptorHandle_GPU(UINT index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE res;
	res.ptr = SRV_Start_Handle_GPU.ptr + index * mCbvSrvUavDescriptorSize;
	return res;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetRTVDescriptorHandle_CPU(UINT index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE res;
	res.ptr = RTV_Start_Handle_CPU.ptr + index * mRtvDescriptorSize;
	return res;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetRTVDescriptorHandle_GPU(UINT index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE res;
	res.ptr = RTV_Start_Handle_GPU.ptr + index * mRtvDescriptorSize;
	return res;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetDSVDescriptorHandle_CPU(UINT index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE res;
	res.ptr = DSV_Start_Handle_CPU.ptr + index * mDsvDescriptorSize;
	return res;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetDSVDescriptorHandle_GPU(UINT index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE res;
	res.ptr = DSV_Start_Handle_GPU.ptr + index * mDsvDescriptorSize;
	return res;
}

