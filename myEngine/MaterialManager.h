#pragma once
#include"Sigleton.h"
#include"Common/d3dUtil.h"
#include"ViewManager.h"
#include"TextureMgr.h"
#include"Common/UploadBuffer.h"
struct Material_Data
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.5f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

	UINT DiffuseMapIndex = 0;
	UINT NormalMapIndex = 0;
	UINT MaterialPad1;
	UINT MaterialPad2;
};
struct Mater_data_pack
{
	ComPtr<ID3D12Resource> resource = nullptr;
	BYTE* mMappedData = nullptr;
};
class MaterialMgr:public Sigleton<MaterialMgr>
{
	friend class Sigleton<MaterialMgr>;
public:
	void initMgr(ComPtr<ID3D12Device> Device, ComPtr<ID3D12GraphicsCommandList> CommandList);
	UINT createMaterial(
		string MaterialName,
		wstring DiffuseTexName=L"",
		wstring NormalTexName = L"",
		XMFLOAT4 DiffuseAlbedo= XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		XMFLOAT3 FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f),
		float Roughness = 0.1f
		);
	bool haveMaterial(string MaterialName);
	UINT GetMaterialIndex(string MaterialName);
	Material GetMaterial(UINT index);
	ComPtr<ID3D12Resource> GetUploadData(UINT index);
private:
	ComPtr<ID3D12Device> md3dDevice;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
	vector<Material> MaterialWhole;
	map<wstring, Texture> name2Texture;
	vector<Mater_data_pack> mDataUpload ;
};
