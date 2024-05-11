#pragma once
#include"Common/d3dUtil.h"
#include"Sigleton.h"
#include"ViewManager.h"
enum TextureStyle
{
	cubeTexture,
	DiffuseTexture,
	NormalTexture
};
class TextureMgr:public Sigleton<TextureMgr>
{
	friend class  Sigleton<TextureMgr>;
public:
	void initTextureMgr(ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandQueue> commandQueue);
	UINT CreateUploadTexture(TextureStyle style,wstring path);
	Texture GetTexture(wstring name);
	bool haveTexture(wstring name);
private:
	map<wstring, Texture> name2Texture;
	ComPtr<ID3D12Device> md3dDevice;
	ComPtr<ID3D12CommandQueue> mCommandQueue;
};
