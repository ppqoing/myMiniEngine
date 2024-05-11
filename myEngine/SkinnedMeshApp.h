#pragma once
#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"
#include "Common/GeometryGenerator.h"
#include "Common/Camera.h"
#include "FrameResource.h"
#include "ShadowMap.h"
#include "Ssao.h"
#include"renderItem.h"
using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;
enum class RenderLayer : int
{
    Opaque = 0,
    SkinnedOpaque,
    Debug,
    Sky,
    Count
};
class SkinnedMeshApp : public D3DApp,  public Sigleton<SkinnedMeshApp>
{
    friend class Sigleton<SkinnedMeshApp>;
public:
    SkinnedMeshApp(const SkinnedMeshApp& rhs) = delete;
    SkinnedMeshApp& operator=(const SkinnedMeshApp& rhs) = delete;
    SkinnedMeshApp();
    ~SkinnedMeshApp();

    virtual bool Initialize(HINSTANCE hInstance)override;

    LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    vector<RenderItem*> GetRenderItem();
    void InitScript();
    void AddInitFunc(void(*func)());
    void AddUpdateFunc(void(*func)());
    void AddDrawFunc(void(*func)());
    void AddDestoryFunc(void(*func)());
    void AddMsgProcFunc(LRESULT(*func)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam));
    HWND GetMainWnd();
    ComPtr<ID3D12Device> GetDevice();
    ComPtr<ID3D12DescriptorHeap> GetSrvHeap();
    ComPtr<ID3D12GraphicsCommandList> GetCommandList();
    float DeltaTime();
private:
    virtual void CreateRtvAndDsvDescriptorHeaps()override;
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
    void AnimateMaterials(const GameTimer& gt);
    void UpdateObjectCBs(const GameTimer& gt);
    void UpdateSkinnedCBs(const GameTimer& gt);
    void UpdateMaterialBuffer(const GameTimer& gt);
    void UpdateShadowTransform(const GameTimer& gt);
    void UpdateMainPassCB(const GameTimer& gt);
    void UpdateShadowPassCB(const GameTimer& gt);
    void UpdateSsaoCB(const GameTimer& gt);

    void LoadTextures();
    void BuildRootSignature();
    void BuildSsaoRootSignature();
    void BuildDescriptorHeaps();
    void BuildShadersAndInputLayout();
    void LoadSkinnedModel();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
    void DrawSceneToShadowMap();
    void DrawNormalsAndDepth();

    CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuSrv(int index)const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrv(int index)const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(int index)const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int index)const;

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12RootSignature> mSsaoRootSignature = nullptr;

    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mSkinnedInputLayout;

    // List of all the render items.
    std::vector<RenderItem*> mAllRitems;

    // Render items divided by PSO.
    std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

    UINT mSkyTexHeapIndex = 0;
    UINT mShadowMapHeapIndex = 0;
    UINT mSsaoHeapIndexStart = 0;
    //   UINT mSsaoAmbientMapIndex = 0;

    UINT mNullCubeSrvIndex = 0;
    UINT mNullTexSrvIndex1 = 0;
    UINT mNullTexSrvIndex2 = 0;

    CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;

    PassConstants mMainPassCB;  // index 0 of pass cbuffer.
    PassConst mShadowPassCB;// index 1 of pass cbuffer.

    //UINT mSkinnedSrvHeapStart = 0;
    //std::string mSkinnedModelFilename = "Models\\soldier.m3d";
    //std::unique_ptr<SkinnedModelInstance> mSkinnedModelInst; 
    //SkinnedData mSkinnedInfo;
    //std::vector<M3DLoader::Subset> mSkinnedSubsets;
    //std::vector<M3DLoader::M3dMaterial> mSkinnedMats;
    //std::vector<std::string> mSkinnedTextureNames;

    Camera mCamera;

    std::unique_ptr<ShadowMap> mShadowMap;

    std::unique_ptr<Ssao> mSsao;

    DirectX::BoundingSphere mSceneBounds;

    float mLightNearZ = 0.0f;
    float mLightFarZ = 0.0f;
    float mTheta = 1.5f * XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;


    XMFLOAT3 mLightPosW;
    XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
    XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
    XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();

    float mLightRotationAngle = 0.0f;
    XMFLOAT3 mBaseLightDirections[3] = {
        XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
        XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
        XMFLOAT3(0.0f, -0.707f, -0.707f)
    };
    XMFLOAT3 mRotatedLightDirections[3];

    POINT mLastMousePos;

    vector<void(*)()> initFunction;
    vector<void(*)()> updateFunction;
    vector<void(*)()> drawFunction;
    vector<void(*)()> destoryFunction;
    vector<LRESULT(*)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)> msgProc;
};
