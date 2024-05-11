#include"TextureMgr.h"

void TextureMgr::initTextureMgr(ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandQueue> commandQueue)
{
	md3dDevice = device;
	mCommandQueue = commandQueue;
	wstring default_diff = L"../Textures/white1x1.dds";
	wstring default_normal = L"../Textures/default_nmap.dds";
	Texture diff, norm;
	ResourceUploadBatch textureUp(md3dDevice.Get());
	textureUp.Begin();
	ThrowIfFailed(CreateDDSTextureFromFile(
		md3dDevice.Get(),
		textureUp,
		default_diff.c_str(),
		diff.Resource.ReleaseAndGetAddressOf()
	));
	ThrowIfFailed(CreateDDSTextureFromFile(
		md3dDevice.Get(),
		textureUp,
		default_normal.c_str(),
		norm.Resource.ReleaseAndGetAddressOf()
	));
	auto uploadFinish = textureUp.End(mCommandQueue.Get());
	uploadFinish.wait();
	D3D12_SHADER_RESOURCE_VIEW_DESC textureSRVdesc = {};
	textureSRVdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	textureSRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	textureSRVdesc.Texture2D.MipLevels = -1;
	textureSRVdesc.Format = diff.Resource->GetDesc().Format;
	diff.SRVheapIndex = DescriptorManager::GetInstance().CreateSRV_CBV_UAV(diff.Resource, textureSRVdesc);
	textureSRVdesc.Format = norm.Resource->GetDesc().Format;
	norm.SRVheapIndex = DescriptorManager::GetInstance().CreateSRV_CBV_UAV(norm.Resource, textureSRVdesc);
	name2Texture[L"defaultDiff"] = diff;
	name2Texture[L"defaultNorm"] = norm;

}

UINT TextureMgr::CreateUploadTexture(TextureStyle style, wstring path)
{
	Texture res;
	ResourceUploadBatch textureUp(md3dDevice.Get());
	textureUp.Begin();
	ThrowIfFailed(CreateDDSTextureFromFile(
		md3dDevice.Get(),
		textureUp,
		path.c_str(),
		res.Resource.ReleaseAndGetAddressOf()
	));
	D3D12_SHADER_RESOURCE_VIEW_DESC textureSRVdesc = {};
	textureSRVdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	textureSRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	if (style==TextureStyle::cubeTexture)
	{
		textureSRVdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	}
	auto uploadFinish = textureUp.End(mCommandQueue.Get());
	uploadFinish.wait();
	textureSRVdesc.Texture2D.MipLevels = -1;
	textureSRVdesc.Format = res.Resource->GetDesc().Format;
	res.SRVheapIndex = DescriptorManager::GetInstance().CreateSRV_CBV_UAV(res.Resource, textureSRVdesc);
	name2Texture[path] = res;

	return res.SRVheapIndex;
}

Texture TextureMgr::GetTexture(wstring name)
{
	if (name2Texture.find(name)==name2Texture.end())
	{
		return name2Texture[L"defaultDiff"];
	}
	return name2Texture[name];
}

bool TextureMgr::haveTexture(wstring name)
{
	if (name2Texture.find(name) == name2Texture.end())
		return false;
	return true;
}
