#include"MaterialManager.h"

void MaterialMgr::initMgr(ComPtr<ID3D12Device> Device, ComPtr<ID3D12GraphicsCommandList> CommandList)
{
	md3dDevice = Device;
	mCommandList = CommandList;

}

UINT MaterialMgr::createMaterial(
	string MaterialName, 
	wstring DiffuseTexName,
	wstring NormalTexName, 
	XMFLOAT4 DiffuseAlbedo,
	XMFLOAT3 FresnelR0,
	float Roughness)
{
	Material mMaterial ;
	mMaterial.Name = MaterialName;
	mMaterial.DiffuseAlbedo = DiffuseAlbedo;
	mMaterial.FresnelR0 = FresnelR0;
	mMaterial.Roughness = Roughness;
	if (DiffuseTexName != L"" && name2Texture.find(DiffuseTexName) == name2Texture.end())
	{
		if (TextureMgr::GetInstance().haveTexture(DiffuseTexName))
		{
			mMaterial.DiffuseSrvHeapIndex = TextureMgr::GetInstance().GetTexture(DiffuseTexName).SRVheapIndex;
		}
		else
		{
			mMaterial.DiffuseSrvHeapIndex = TextureMgr::GetInstance().CreateUploadTexture(TextureStyle::DiffuseTexture, DiffuseTexName);
		}
	}
	else
	{
		mMaterial.DiffuseSrvHeapIndex = TextureMgr::GetInstance().GetTexture(L"defaultDiff").SRVheapIndex;
	}
	if (NormalTexName != L"" && name2Texture.find(NormalTexName) == name2Texture.end())
	{
		if (TextureMgr::GetInstance().haveTexture(NormalTexName))
		{
			mMaterial.NormalSrvHeapIndex = TextureMgr::GetInstance().GetTexture(NormalTexName).SRVheapIndex;
		}
		else
		{
			mMaterial.NormalSrvHeapIndex = TextureMgr::GetInstance().CreateUploadTexture(TextureStyle::NormalTexture, NormalTexName);
		}
	}
	else
	{
		mMaterial.NormalSrvHeapIndex = TextureMgr::GetInstance().GetTexture(L"defaultNorm").SRVheapIndex;
	}
	MaterialWhole.push_back(mMaterial);
	Material_Data data_m;
	data_m.DiffuseAlbedo = mMaterial.DiffuseAlbedo;
	data_m.FresnelR0 = mMaterial.FresnelR0;
	data_m.Roughness = mMaterial.Roughness;
	data_m.DiffuseMapIndex = mMaterial.DiffuseSrvHeapIndex;
	data_m.NormalMapIndex = mMaterial.NormalSrvHeapIndex;

	Mater_data_pack data;
	size_t szAlign = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(d3dUtil::CalcConstantBufferByteSize(sizeof(Material_Data), szAlign)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&data.resource)));

	ThrowIfFailed(data.resource->Map(0, nullptr, reinterpret_cast<void**>(&data.mMappedData)));
	auto t = sizeof(Material_Data);
	auto m = d3dUtil::CalcConstantBufferByteSize(sizeof(Material_Data), szAlign);
	memcpy(data.mMappedData, &data_m, sizeof(Material_Data));

	mDataUpload.push_back(data);
	return MaterialWhole.size() - 1;
}

bool MaterialMgr::haveMaterial(string MaterialName)
{
	for (size_t i = 0; i < MaterialWhole.size(); i++)
	{
		if (MaterialWhole[i].Name == MaterialName)
			return true;
	}
	return false;
}

UINT MaterialMgr::GetMaterialIndex(string MaterialName)
{
	for (size_t i = 0; i < MaterialWhole.size(); i++)
	{
		if (MaterialWhole[i].Name == MaterialName)
			return i;
	}
	return 0;
}

Material MaterialMgr::GetMaterial(UINT index)
{
	return MaterialWhole[index];
}

ComPtr<ID3D12Resource> MaterialMgr::GetUploadData(UINT index)
{
	return mDataUpload[index].resource;
}

