//***************************************************************************************
// SkinnedMeshApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************
#include"SkinnedMeshApp.h"
#include"logSystem/logSystem.h"
#include"resource/resourceUtil.h"
const int gNumFrameResources = 3;

//
//void CreateConsoleWindow()
//{
//    AllocConsole();
//    FILE* stream;
//    freopen_s(&stream, "CON", "r", stdin);
//    freopen_s(&stream, "CON", "w", stdout);
//}
//
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
//    PSTR cmdLine, int showCmd)
//{
//    // Enable run-time memory check for debug builds.
//#if defined(DEBUG) | defined(_DEBUG)
//    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//#endif
//
//    try
//    {
//        CreateConsoleWindow();
//        SkinnedMeshApp theApp(hInstance);
//        if(!theApp.Initialize())
//            return 0;
//
//        return theApp.Run();
//    }
//    catch(DxException& e)
//    {
//        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
//        return 0;
//    }
//}

SkinnedMeshApp::SkinnedMeshApp()
    : D3DApp()
{
    // Estimate the scene bounding sphere manually since we know how the scene was constructed.
    // The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
    // the world space origin.  In general, you need to loop over every world space vertex
    // position and compute the bounding sphere.
    mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
    mSceneBounds.Radius = sqrtf(10.0f*10.0f + 15.0f*15.0f);
}

bool SkinnedMeshApp::Initialize(HINSTANCE hInstance)
   
{
    mhAppInst = hInstance;
    if(!D3DApp::Initialize(hInstance))
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCamera.SetPosition(0.0f, 2.0f, -15.0f);
    DescriptorManager::GetInstance().init(md3dDevice);
    TextureMgr::GetInstance().initTextureMgr(md3dDevice, mCommandQueue);
    MaterialMgr::GetInstance().initMgr(md3dDevice, mCommandList);

    mShadowMap = std::make_unique<ShadowMap>(md3dDevice.Get(),
        2048, 2048);
    mSsao = std::make_unique<Ssao>(
        md3dDevice.Get(),
        mCommandList.Get(),
        mClientWidth, mClientHeight);

    LoadSkinnedModel();
	LoadTextures();
    BuildRootSignature();
    BuildSsaoRootSignature();
	BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
	BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    mSsao->SetPSOs(mPSOs["ssao"].Get(), mPSOs["ssaoBlur"].Get());

    //renderItemResource::GetInstance().clean();
    //for (size_t i = 0; i < mRitemLayer[(int)RenderLayer::Opaque].size(); i++)
    //{
    //    mRitemLayer[(int)RenderLayer::Opaque][i];
    //    renderItemResource::GetInstance().AddboxRes();
    //    renderItemResource::GetInstance().AddCylinder();
    //    renderItemResource::GetInstance().AddGrid();
    //    renderItemResource::GetInstance().AddQuad();
    //    renderItemResource::GetInstance().AddSphere();
    //}
    //for (size_t i = 0; i < mRitemLayer[(int)RenderLayer::Sky].size(); i++)
    //{

    //}
    //for (size_t i = 0; i < mRitemLayer[(int)RenderLayer::SkinnedOpaque].size(); i++)
    //{
    //    skinModel_res temp;
    //    temp.filePath = mRitemLayer[(int)RenderLayer::SkinnedOpaque][i]->m_strFileName;
    //    temp.name = mRitemLayer[(int)RenderLayer::SkinnedOpaque][i]->m_strFileName;
    //    temp.setTransform(mRitemLayer[(int)RenderLayer::SkinnedOpaque][i]->m_ModelTranformation);
    //    temp.setTex(mRitemLayer[(int)RenderLayer::SkinnedOpaque][i]->TexTransform);
    //    renderItemResource::GetInstance().AddskinModel_res(temp);
    //}
    //renderItemResource::GetInstance().Save2File();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();
    LogSystem::GetInstance().log(LogSystem::LogLevel::info, "init dx12");
    return true;
}

void SkinnedMeshApp::CreateRtvAndDsvDescriptorHeaps()
{
    D3DApp::CreateRtvAndDsvDescriptorHeaps();
    // Add +1 for screen normal map, +2 for ambient maps.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 3;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

    // Add +1 DSV for shadow map.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 2;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}
 
void SkinnedMeshApp::OnResize()
{
    D3DApp::OnResize();

	mCamera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

    if(mSsao != nullptr)
    {
        mSsao->OnResize(mClientWidth, mClientHeight);

        // Resources changed, so need to rebuild descriptors.
        mSsao->RebuildDescriptors(mDepthStencilBuffer.Get());
    }
}

void SkinnedMeshApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    //
    // Animate the lights (and hence shadows).
    //

    mLightRotationAngle += 0.1f*gt.DeltaTime();

    XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
    for(int i = 0; i < 3; ++i)
    {
        XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
        lightDir = XMVector3TransformNormal(lightDir, R);
        XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
    }
 
	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
    UpdateSkinnedCBs(gt);
	UpdateMaterialBuffer(gt);
    UpdateShadowTransform(gt);
	UpdateMainPassCB(gt);
    UpdateShadowPassCB(gt);
    UpdateSsaoCB(gt);
    for (size_t i = 0; i < updateFunction.size(); i++)
    {
        updateFunction[i]();
    }
}

void SkinnedMeshApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

    //ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
    ID3D12DescriptorHeap* descriptorHeaps[] = { DescriptorManager::GetInstance().GetSRVDescriptorHeap().Get()};

    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	//
	// Shadow map pass.
	//

    // Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
    // set as a root descriptor.
    //auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
    //mCommandList->SetGraphicsRootShaderResourceView(3, matBuffer->GetGPUVirtualAddress());
	
    // Bind null SRV for shadow map pass.
   // mCommandList->SetGraphicsRootDescriptorTable(4, mNullSrv);	 
    
    
    mCommandList->SetGraphicsRootDescriptorTable(4, DescriptorManager::GetInstance().GetSRVDescriptorHandle_GPU(mNullCubeSrvIndex));
    mCommandList->SetGraphicsRootDescriptorTable(5, DescriptorManager::GetInstance().GetSRVDescriptorHandle_GPU(mNullTexSrvIndex1));
    mCommandList->SetGraphicsRootDescriptorTable(6, DescriptorManager::GetInstance().GetSRVDescriptorHandle_GPU(mNullTexSrvIndex2));

    // Bind all the textures used in this scene.  Observe
    // that we only have to specify the first descriptor in the table.  
    // The root signature knows how many descriptors are expected in the table.
   // mCommandList->SetGraphicsRootDescriptorTable(5, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    DrawSceneToShadowMap();

	//
	// Normal/depth pass.
	//
	
	DrawNormalsAndDepth();
	
	//
	//
	// 
	
    mCommandList->SetGraphicsRootSignature(mSsaoRootSignature.Get());
    mSsao->ComputeSsao(mCommandList.Get(), mCurrFrameResource, 2);
	
	//
	// Main rendering pass.
	//
	
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    // Rebind state whenever graphics root signature changes.

    // Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
    // set as a root descriptor.
    //matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
    //mCommandList->SetGraphicsRootShaderResourceView(3, matBuffer->GetGPUVirtualAddress());
    UINT cubemapindex = TextureMgr::GetInstance().GetTexture(L"../Textures/desertcube1024.dds").SRVheapIndex;
    mCommandList->SetGraphicsRootDescriptorTable(4,DescriptorManager::GetInstance().GetSRVDescriptorHandle_GPU(cubemapindex));
    mCommandList->SetGraphicsRootDescriptorTable(5, mShadowMap.get()->Srv());
    mCommandList->SetGraphicsRootDescriptorTable(6, mSsao.get()->AmbientMapSrv());


    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	// Bind all the textures used in this scene.  Observe
    // that we only have to specify the first descriptor in the table.  
    // The root signature knows how many descriptors are expected in the table.
    //mCommandList->SetGraphicsRootDescriptorTable(5, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	
    auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    // Bind the sky cube map.  For our demos, we just use one "world" cube map representing the environment
    // from far away, so all objects will use the same cube map and we only need to set it once per-frame.  
    // If we wanted to use "local" cube maps, we would have to change them per-object, or dynamically
    // index into an array of cube maps.

  /*  CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    skyTexDescriptor.Offset(mSkyTexHeapIndex, mCbvSrvUavDescriptorSize);
    mCommandList->SetGraphicsRootDescriptorTable(4, skyTexDescriptor);*/

    mCommandList->SetPipelineState(mPSOs["opaque"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

    mCommandList->SetPipelineState(mPSOs["skinnedOpaque"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::SkinnedOpaque]);

    //mCommandList->SetPipelineState(mPSOs["debug"].Get());
    //DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Debug]);

	mCommandList->SetPipelineState(mPSOs["sky"].Get());
    mCommandList->SetGraphicsRootDescriptorTable(4, DescriptorManager::GetInstance().GetSRVDescriptorHandle_GPU(cubemapindex));
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Sky]);

    for (size_t i = 0; i < drawFunction.size(); i++)
    {
        drawFunction[i]();
    }

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void SkinnedMeshApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void SkinnedMeshApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void SkinnedMeshApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}
 
void SkinnedMeshApp::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if(GetAsyncKeyState('W') & 0x8000)
		mCamera.Walk(10.0f*dt);

	if(GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-10.0f*dt);

	if(GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-10.0f*dt);

	if(GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(10.0f*dt);

	mCamera.UpdateViewMatrix();
}
 
void SkinnedMeshApp::AnimateMaterials(const GameTimer& gt)
{
	
}

void SkinnedMeshApp::UpdateObjectCBs(const GameTimer& gt)
{
    for (size_t i = 0; i < mAllRitems.size(); i++)
    {
        mAllRitems[i]->UpdateCB();
    }
}

void SkinnedMeshApp::UpdateSkinnedCBs(const GameTimer& gt)
{
    //auto currSkinnedCB = mCurrFrameResource->SkinnedCB.get();
   
    //// We only have one skinned model being animated.
    //mSkinnedModelInst->UpdateSkinnedAnimation(gt.DeltaTime());
    //    
    //SkinnedConstants skinnedConstants;
    //std::copy(
    //    std::begin(mSkinnedModelInst->FinalTransforms),
    //    std::end(mSkinnedModelInst->FinalTransforms),
    //    &skinnedConstants.BoneTransforms[0]);

    //currSkinnedCB->CopyData(0, skinnedConstants);
    for (size_t i = 0; i < mAllRitems.size(); i++)
    {
        bool s = mAllRitems[i]->IsBaseObj;
        mAllRitems[i]->BoneAnimePlay(0, gt.DeltaTime());
    }
}
 
void SkinnedMeshApp::UpdateMaterialBuffer(const GameTimer& gt)
{
	//auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
	//for(auto& e : mMaterials)
	//{
	//	// Only update the cbuffer data if the constants have changed.  If the cbuffer
	//	// data changes, it needs to be updated for each FrameResource.
	//	Material* mat = e.second.get();
	//	if(mat->NumFramesDirty > 0)
	//	{
	//		XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

	//		MaterialData matData;
	//		matData.DiffuseAlbedo = mat->DiffuseAlbedo;
	//		matData.FresnelR0 = mat->FresnelR0;
	//		matData.Roughness = mat->Roughness;
	//		XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
	//		matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
	//		matData.NormalMapIndex = mat->NormalSrvHeapIndex;

	//		currMaterialBuffer->CopyData(mat->MatCBIndex, matData);

	//		// Next FrameResource need to be updated too.
	//		mat->NumFramesDirty--;
	//	}
	//}
}

void SkinnedMeshApp::UpdateShadowTransform(const GameTimer& gt)
{
    // Only the first "main" light casts a shadow.
    XMVECTOR lightDir = XMLoadFloat3(&mRotatedLightDirections[0]);
    XMVECTOR lightPos = -2.0f*mSceneBounds.Radius*lightDir;
    XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
    XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

    XMStoreFloat3(&mLightPosW, lightPos);

    // Transform bounding sphere to light space.
    XMFLOAT3 sphereCenterLS;
    XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

    // Ortho frustum in light space encloses scene.
    float l = sphereCenterLS.x - mSceneBounds.Radius;
    float b = sphereCenterLS.y - mSceneBounds.Radius;
    float n = sphereCenterLS.z - mSceneBounds.Radius;
    float r = sphereCenterLS.x + mSceneBounds.Radius;
    float t = sphereCenterLS.y + mSceneBounds.Radius;
    float f = sphereCenterLS.z + mSceneBounds.Radius;

    mLightNearZ = n;
    mLightFarZ = f;
    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);

    XMMATRIX S = lightView*lightProj*T;
    XMStoreFloat4x4(&mLightView, lightView);
    XMStoreFloat4x4(&mLightProj, lightProj);
    XMStoreFloat4x4(&mShadowTransform, S);
}

void SkinnedMeshApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);

    XMMATRIX viewProjTex = XMMatrixMultiply(viewProj, T);
    XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProjTex, XMMatrixTranspose(viewProjTex));
    XMStoreFloat4x4(&mMainPassCB.ShadowTransform, XMMatrixTranspose(shadowTransform));
	mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = mRotatedLightDirections[0];
	mMainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.7f };
	mMainPassCB.Lights[1].Direction = mRotatedLightDirections[1];
	mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	mMainPassCB.Lights[2].Direction = mRotatedLightDirections[2];
	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };
 
	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void SkinnedMeshApp::UpdateShadowPassCB(const GameTimer& gt)
{
    XMMATRIX view = XMLoadFloat4x4(&mLightView);
    XMMATRIX proj = XMLoadFloat4x4(&mLightProj);

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    UINT w = mShadowMap->Width();
    UINT h = mShadowMap->Height();

    XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    mShadowPassCB.EyePosW = mLightPosW;
    mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
    mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
    mShadowPassCB.NearZ = mLightNearZ;
    mShadowPassCB.FarZ = mLightFarZ;
    mShadowMap->UpdatePassCB(mShadowPassCB);
    //auto currPassCB = mShadowMap->PassCB;
    //currPassCB->CopyData(1, mShadowPassCB);
}

void SkinnedMeshApp::UpdateSsaoCB(const GameTimer& gt)
{
    SsaoConstants ssaoCB;

    XMMATRIX P = mCamera.GetProj();

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);

    ssaoCB.Proj    = mMainPassCB.Proj;
    ssaoCB.InvProj = mMainPassCB.InvProj;
    XMStoreFloat4x4(&ssaoCB.ProjTex, XMMatrixTranspose(P*T));

    mSsao->GetOffsetVectors(ssaoCB.OffsetVectors);

    auto blurWeights = mSsao->CalcGaussWeights(2.5f);
    ssaoCB.BlurWeights[0] = XMFLOAT4(&blurWeights[0]);
    ssaoCB.BlurWeights[1] = XMFLOAT4(&blurWeights[4]);
    ssaoCB.BlurWeights[2] = XMFLOAT4(&blurWeights[8]);

    ssaoCB.InvRenderTargetSize = XMFLOAT2(1.0f / mSsao->SsaoMapWidth(), 1.0f / mSsao->SsaoMapHeight());

    // Coordinates given in view space.
    ssaoCB.OcclusionRadius = 0.5f;
    ssaoCB.OcclusionFadeStart = 0.2f;
    ssaoCB.OcclusionFadeEnd = 2.0f;
    ssaoCB.SurfaceEpsilon = 0.05f;

    auto currSsaoCB = mCurrFrameResource->SsaoCB.get();
    currSsaoCB->CopyData(0, ssaoCB);
}

void SkinnedMeshApp::LoadTextures()
{
	//std::vector<std::string> texNames = 
	//{
	//	"bricksDiffuseMap",
	//	"bricksNormalMap",
	//	"tileDiffuseMap",
	//	"tileNormalMap",
	//	"defaultDiffuseMap",
	//	"defaultNormalMap",
	//	"skyCubeMap"
	//};
	
	std::vector<std::wstring> texFilenames = 
	{
		L"../Textures/bricks2.dds",
		L"../Textures/bricks2_nmap.dds",
		L"../Textures/tile.dds",
		L"../Textures/tile_nmap.dds",
		L"../Textures/white1x1.dds",
		L"../Textures/default_nmap.dds",
		L"../Textures/desertcube1024.dds"
	};
    TextureMgr::GetInstance().CreateUploadTexture(TextureStyle::DiffuseTexture, texFilenames[0]);
    TextureMgr::GetInstance().CreateUploadTexture(TextureStyle::DiffuseTexture, texFilenames[1]);
    TextureMgr::GetInstance().CreateUploadTexture(TextureStyle::NormalTexture, texFilenames[2]);
    TextureMgr::GetInstance().CreateUploadTexture(TextureStyle::NormalTexture, texFilenames[3]);
    TextureMgr::GetInstance().CreateUploadTexture(TextureStyle::cubeTexture, texFilenames[6]);

    /*// Add skinned model textures to list so we can reference by name later.
    for(UINT i = 0; i < mSkinnedMats.size(); ++i)
    {
        std::string diffuseName = mSkinnedMats[i].DiffuseMapName;
        std::string normalName = mSkinnedMats[i].NormalMapName;

        std::wstring diffuseFilename = L"../../Textures/" + AnsiToWString(diffuseName);
        std::wstring normalFilename = L"../../Textures/" + AnsiToWString(normalName);

        // strip off extension
        diffuseName = diffuseName.substr(0, diffuseName.find_last_of("."));
        normalName = normalName.substr(0, normalName.find_last_of("."));

        mSkinnedTextureNames.push_back(diffuseName);
        texNames.push_back(diffuseName);
        texFilenames.push_back(diffuseFilename);

        mSkinnedTextureNames.push_back(normalName);
        texNames.push_back(normalName);
        texFilenames.push_back(normalFilename);
    }
	
	for(int i = 0; i < (int)texNames.size(); ++i)
	{
        // Don't create duplicates.
        if(mTextures.find(texNames[i]) == std::end(mTextures))
        {
            auto texMap = std::make_unique<Texture>();
            texMap->Name = texNames[i];
            texMap->Filename = texFilenames[i];
            ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
                mCommandList.Get(), texMap->Filename.c_str(),
                texMap->Resource, texMap->UploadHeap));

            mTextures[texMap->Name] = std::move(texMap);
        }
	} */ 
}

void SkinnedMeshApp::BuildRootSignature()
{
    myRootSignature default_RootSignature(md3dDevice,rootSignatureStyle::default_RSS);
    mRootSignature = default_RootSignature.GetRootSignature();
}

void SkinnedMeshApp::BuildSsaoRootSignature()
{
    myRootSignature ssaoRSS(md3dDevice, rootSignatureStyle::SSAO_RSS);
    mSsaoRootSignature = ssaoRSS.GetRootSignature();
}

void SkinnedMeshApp::BuildDescriptorHeaps()
{
    /*
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 64;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	std::vector<ComPtr<ID3D12Resource>> tex2DList = 
	{
		mTextures["bricksDiffuseMap"]->Resource,
		mTextures["bricksNormalMap"]->Resource,
		mTextures["tileDiffuseMap"]->Resource,
		mTextures["tileNormalMap"]->Resource,
		mTextures["defaultDiffuseMap"]->Resource,
		mTextures["defaultNormalMap"]->Resource
	};

    mSkinnedSrvHeapStart = (UINT)tex2DList.size();

    for(UINT i = 0; i < (UINT)mSkinnedTextureNames.size(); ++i)
    {
        auto texResource = mTextures[mSkinnedTextureNames[i]]->Resource;
        assert(texResource != nullptr);
        tex2DList.push_back(texResource);
    }
	

	auto skyCubeMap = mTextures["skyCubeMap"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	
	for(UINT i = 0; i < (UINT)tex2DList.size(); ++i)
	{
		srvDesc.Format = tex2DList[i]->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = tex2DList[i]->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(tex2DList[i].Get(), &srvDesc, hDescriptor);
		// next descriptor
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}
	
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = skyCubeMap->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = skyCubeMap->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(skyCubeMap.Get(), &srvDesc, hDescriptor);
	
	mSkyTexHeapIndex = (UINT)tex2DList.size();  //6
    mShadowMapHeapIndex = mSkyTexHeapIndex + 1;
    mSsaoHeapIndexStart = mShadowMapHeapIndex + 1;
    mSsaoAmbientMapIndex = mSsaoHeapIndexStart + 3;
    mNullCubeSrvIndex = mSsaoHeapIndexStart + 5;
    mNullTexSrvIndex1 = mNullCubeSrvIndex + 1;
    mNullTexSrvIndex2 = mNullTexSrvIndex1 + 1;

    auto nullSrv = GetCpuSrv(mNullCubeSrvIndex);
    mNullSrv = GetGpuSrv(mNullCubeSrvIndex);

    md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
    nullSrv.Offset(1, mCbvSrvUavDescriptorSize);

    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

    nullSrv.Offset(1, mCbvSrvUavDescriptorSize);
    md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);*/
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels =-1;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    srvDesc.Format = TextureMgr::GetInstance().GetTexture(L"../Textures/desertcube1024.dds").Resource->GetDesc().Format;
   mNullCubeSrvIndex= DescriptorManager::GetInstance().CreateSRV_CBV_UAV(nullptr, srvDesc);
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
   mNullTexSrvIndex1= DescriptorManager::GetInstance().CreateSRV_CBV_UAV(nullptr, srvDesc);
    mNullTexSrvIndex2= DescriptorManager::GetInstance().CreateSRV_CBV_UAV(nullptr, srvDesc);

    mShadowMap->BuildDescriptors();

    mSsao->RebuildDescriptors(mDepthStencilBuffer.Get());
}

void SkinnedMeshApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

    const D3D_SHADER_MACRO skinnedDefines[] =
    {
        "SKINNED", "1",
        NULL, NULL
    };

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["skinnedVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", skinnedDefines, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");
    mShaders["skinnedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", skinnedDefines, "PS", "ps_5_1");


    mShaders["shadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["skinnedShadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", skinnedDefines, "VS", "vs_5_1");
    mShaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
    mShaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");
	
    mShaders["debugVS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["debugPS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["drawNormalsVS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["skinnedDrawNormalsVS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", skinnedDefines, "VS", "vs_5_1");
    mShaders["drawNormalsPS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["ssaoVS"] = d3dUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["ssaoPS"] = d3dUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "PS", "ps_5_1");

    mShaders["ssaoBlurVS"] = d3dUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["ssaoBlurPS"] = d3dUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    mSkinnedInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "WEIGHTS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BONEINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}


void SkinnedMeshApp::LoadSkinnedModel()
{
    const string path = "model\\lxq\\lxq.X";
    RenderItem* skinModel = new RenderItem();
    skinModel->initAsSkinModel(path);
    skinModel->UploadModel(md3dDevice.Get(), mCommandQueue.Get(), mDirectCmdListAlloc.Get(), mCommandList.Get());
    skinModel->SetScale(XMFLOAT3(0.5,0.5,0.5));
    skinModel->AddScript("Class1");
    mAllRitems.push_back(skinModel);
    mRitemLayer[(int)RenderLayer::SkinnedOpaque].push_back(skinModel);
	/*std::vector<M3DLoader::SkinnedVertex> vertices;
	std::vector<std::uint16_t> indices;	
 
	M3DLoader m3dLoader;
	m3dLoader.LoadM3d(mSkinnedModelFilename, vertices, indices, 
        mSkinnedSubsets, mSkinnedMats, mSkinnedInfo);

    mSkinnedModelInst = std::make_unique<SkinnedModelInstance>();
    mSkinnedModelInst->SkinnedInfo = &mSkinnedInfo;
    mSkinnedModelInst->FinalTransforms.resize(mSkinnedInfo.BoneCount());
    mSkinnedModelInst->ClipName = "Take1";
    mSkinnedModelInst->TimePos = 0.0f;
 
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(SkinnedVertex);
    const UINT ibByteSize = (UINT)indices.size()  * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = mSkinnedModelFilename;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(SkinnedVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	for(UINT i = 0; i < (UINT)mSkinnedSubsets.size(); ++i)
	{
		SubmeshGeometry submesh;
		std::string name = "sm_" + std::to_string(i);
		
        submesh.IndexCount = (UINT)mSkinnedSubsets[i].FaceCount * 3;
        submesh.StartIndexLocation = mSkinnedSubsets[i].FaceStart * 3;
        submesh.BaseVertexLocation = 0;

		geo->DrawArgs[name] = submesh;
	}

	mGeometries[geo->Name] = std::move(geo);*/
}

void SkinnedMeshApp::BuildPSOs() 
{
    auto pso_tem=psoResource::GetInstance().GetPsoResource();
    for (size_t i = 0; i < pso_tem.psoRes.size(); i++)
    {
        myPso temp(md3dDevice, pso_tem.psoRes[i].style, mShaders[pso_tem.psoRes[i].VsName], mShaders[pso_tem.psoRes[i].PsName]);
        mPSOs[pso_tem.psoRes[i].name] = temp.GetPipeline();
    }
}

void SkinnedMeshApp::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),2 ));
    }
}

void SkinnedMeshApp::BuildMaterials()
{

    auto materials = MaterialResource::GetInstance().GetMaterial();
    for (size_t i = 0; i < materials.m_all.size(); i++)
    {
        MaterialMgr::GetInstance().createMaterial(
            materials.m_all[i].m_name,
            materials.m_all[i].DiffuseTexName,
            materials.m_all[i].NormalTexName,
            XMFLOAT4(materials.m_all[i].DiffuseAlbedo_x, materials.m_all[i].DiffuseAlbedo_y, materials.m_all[i].DiffuseAlbedo_z, materials.m_all[i].DiffuseAlbedo_w),
            XMFLOAT3(materials.m_all[i].FresnelR0_x, materials.m_all[i].FresnelR0_y, materials.m_all[i].FresnelR0_z),
            materials.m_all[i].Roughness
        );

    }
}

void SkinnedMeshApp::BuildRenderItems()
{
    //auto render_all = renderItemResource::GetInstance().GetRenderItem();

    RenderItem* sky_item=new RenderItem;
    sky_item->initAsSphere(0.5f, 20, 20);
    sky_item->UploadModel(md3dDevice.Get(), mCommandQueue.Get(), mDirectCmdListAlloc.Get(), mCommandList.Get());
    XMStoreFloat4x4(&sky_item->m_ModelTranformation, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
    sky_item->SetMaterial("sky");
    mAllRitems.push_back(sky_item);
    mRitemLayer[(int)RenderLayer::Sky].push_back(sky_item);

    //for (size_t i = 0; i < render_all.boxs.size(); i++)
    //{
    //    RenderItem* box_Item = new RenderItem;
    //    box_Item->initAsBox(render_all.boxs[i].width, render_all.boxs[i].height, render_all.boxs[i].depth, render_all.boxs[i].numSubdivisions);
    //    box_Item->UploadModel(md3dDevice.Get(), mCommandQueue.Get(), mDirectCmdListAlloc.Get(), mCommandList.Get());
    //    box_Item->SetMaterial(render_all.boxs[i].material);
    //    mAllRitems.push_back(box_Item);
    //    mRitemLayer[(int)RenderLayer::Opaque].push_back(box_Item);
    //}

    RenderItem* brick0_item = new RenderItem;
    brick0_item->initAsQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    brick0_item->UploadModel(md3dDevice.Get(), mCommandQueue.Get(), mDirectCmdListAlloc.Get(), mCommandList.Get());
    brick0_item->SetMaterial("bricks0");
    mAllRitems.push_back(brick0_item);
    mRitemLayer[(int)RenderLayer::Debug].push_back(brick0_item);


    RenderItem* box_Item = new RenderItem;
    box_Item->initAsBox(1.0f, 1.0f, 1.0f, 3);
    box_Item->UploadModel(md3dDevice.Get(), mCommandQueue.Get(), mDirectCmdListAlloc.Get(), mCommandList.Get());
    box_Item->SetMaterial("bricks0");
    XMStoreFloat4x4(&box_Item->m_ModelTranformation, XMMatrixScaling(2.0f, 1.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    XMStoreFloat4x4(&box_Item->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
    mAllRitems.push_back(box_Item);
    mRitemLayer[(int)RenderLayer::Opaque].push_back(box_Item);


    RenderItem* grid_item = new RenderItem;
    grid_item->initAsBox(1.0f, 1.0f, 1.0f, 3);
    grid_item->UploadModel(md3dDevice.Get(), mCommandQueue.Get(), mDirectCmdListAlloc.Get(), mCommandList.Get());
    grid_item->SetMaterial("tile0");
    grid_item->SetScale(XMFLOAT3(30, 0.1, 30));
    XMStoreFloat4x4(&grid_item->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
    mAllRitems.push_back(grid_item);
    mRitemLayer[(int)RenderLayer::Opaque].push_back(grid_item);


    XMMATRIX brickTexTransform = XMMatrixScaling(1.5f, 2.0f, 1.0f);
    UINT objCBIndex = 4;
    for (int i = 0; i < 5; ++i)
    {
        RenderItem* leftCylRitem = new RenderItem;
        RenderItem* rightCylRitem = new RenderItem;
        RenderItem* leftSphereRitem = new RenderItem;
        RenderItem* rightSphereRitem = new RenderItem;
        XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
        XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

        XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
        XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);



        XMStoreFloat4x4(&leftCylRitem->m_ModelTranformation, rightCylWorld);
        XMStoreFloat4x4(&leftCylRitem->TexTransform, brickTexTransform);
        leftCylRitem->initAsCylinder(0.5f, 0.3f, 3.0f, 20, 20);
        leftCylRitem->UploadModel(md3dDevice.Get(), mCommandQueue.Get(), mDirectCmdListAlloc.Get(), mCommandList.Get());
        leftCylRitem->SetMaterial("bricks0");

        XMStoreFloat4x4(&rightCylRitem->m_ModelTranformation, leftCylWorld);
        XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
        rightCylRitem->initAsCylinder(0.5f, 0.3f, 3.0f, 20, 20);
        rightCylRitem->UploadModel(md3dDevice.Get(), mCommandQueue.Get(), mDirectCmdListAlloc.Get(), mCommandList.Get());
        rightCylRitem->SetMaterial("bricks0");

        XMStoreFloat4x4(&leftSphereRitem->m_ModelTranformation, leftSphereWorld);
        leftSphereRitem->initAsSphere(0.5f, 20, 20);
        leftSphereRitem->UploadModel(md3dDevice.Get(), mCommandQueue.Get(), mDirectCmdListAlloc.Get(), mCommandList.Get());
        leftSphereRitem->SetMaterial("mirror0");


        XMStoreFloat4x4(&rightSphereRitem->m_ModelTranformation, rightSphereWorld);
        rightSphereRitem->initAsSphere(0.5f, 20, 20);
        rightSphereRitem->UploadModel(md3dDevice.Get(), mCommandQueue.Get(), mDirectCmdListAlloc.Get(), mCommandList.Get());
        rightSphereRitem->SetMaterial("mirror0");
;

        mRitemLayer[(int)RenderLayer::Opaque].push_back(leftCylRitem);
        mRitemLayer[(int)RenderLayer::Opaque].push_back(rightCylRitem);
        mRitemLayer[(int)RenderLayer::Opaque].push_back(leftSphereRitem);
        mRitemLayer[(int)RenderLayer::Opaque].push_back(rightSphereRitem);

        mAllRitems.push_back(leftCylRitem);
        mAllRitems.push_back(rightCylRitem);
        mAllRitems.push_back(leftSphereRitem);
        mAllRitems.push_back(rightSphereRitem);
    }

	/*
    auto skyRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skyRitem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
	skyRitem->TexTransform = MathHelper::Identity4x4();
	skyRitem->ObjCBIndex = 0;
	skyRitem->Mat = mMaterials["sky"].get();
	skyRitem->Geo = mGeometries["shapeGeo"].get();
	skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyRitem->IndexCount = skyRitem->Geo->DrawArgs["sphere"].IndexCount;
	skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Sky].push_back(skyRitem.get());
	mAllRitems.push_back(std::move(skyRitem));
    
    auto quadRitem = std::make_unique<RenderItem>();
    quadRitem->World = MathHelper::Identity4x4();
    quadRitem->TexTransform = MathHelper::Identity4x4();
    quadRitem->ObjCBIndex = 1;
    quadRitem->Mat = mMaterials["bricks0"].get();
    quadRitem->Geo = mGeometries["shapeGeo"].get();
    quadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    quadRitem->IndexCount = quadRitem->Geo->DrawArgs["quad"].IndexCount;
    quadRitem->StartIndexLocation = quadRitem->Geo->DrawArgs["quad"].StartIndexLocation;
    quadRitem->BaseVertexLocation = quadRitem->Geo->DrawArgs["quad"].BaseVertexLocation;

    mRitemLayer[(int)RenderLayer::Debug].push_back(quadRitem.get());
    mAllRitems.push_back(std::move(quadRitem));

    auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 1.0f, 2.0f)*XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	boxRitem->ObjCBIndex = 2;
	boxRitem->Mat = mMaterials["bricks0"].get();
	boxRitem->Geo = mGeometries["shapeGeo"].get();
	boxRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
	mAllRitems.push_back(std::move(boxRitem));
    
        auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->World = MathHelper::Identity4x4();
    XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
    gridRitem->ObjCBIndex = 3;
    gridRitem->Mat = mMaterials["tile0"].get();
    gridRitem->Geo = mGeometries["shapeGeo"].get();
    gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

    mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
    mAllRitems.push_back(std::move(gridRitem));


    XMMATRIX brickTexTransform = XMMatrixScaling(1.5f, 2.0f, 1.0f);
    UINT objCBIndex = 4;
    for(int i = 0; i < 5; ++i)
    {
        auto leftCylRitem = std::make_unique<RenderItem>();
        auto rightCylRitem = std::make_unique<RenderItem>();
        auto leftSphereRitem = std::make_unique<RenderItem>();
        auto rightSphereRitem = std::make_unique<RenderItem>();

        XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f);
        XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f);

        XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f);
        XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f);

        XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
        XMStoreFloat4x4(&leftCylRitem->TexTransform, brickTexTransform);
        leftCylRitem->ObjCBIndex = objCBIndex++;
        leftCylRitem->Mat = mMaterials["bricks0"].get();
        leftCylRitem->Geo = mGeometries["shapeGeo"].get();
        leftCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
        leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

        XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
        XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
        rightCylRitem->ObjCBIndex = objCBIndex++;
        rightCylRitem->Mat = mMaterials["bricks0"].get();
        rightCylRitem->Geo = mGeometries["shapeGeo"].get();
        rightCylRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
        rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

        XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
        leftSphereRitem->TexTransform = MathHelper::Identity4x4();
        leftSphereRitem->ObjCBIndex = objCBIndex++;
        leftSphereRitem->Mat = mMaterials["mirror0"].get();
        leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
        leftSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
        leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
        leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

        XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
        rightSphereRitem->TexTransform = MathHelper::Identity4x4();
        rightSphereRitem->ObjCBIndex = objCBIndex++;
        rightSphereRitem->Mat = mMaterials["mirror0"].get();
        rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
        rightSphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
        rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
        rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

        mRitemLayer[(int)RenderLayer::Opaque].push_back(leftCylRitem.get());
        mRitemLayer[(int)RenderLayer::Opaque].push_back(rightCylRitem.get());
        mRitemLayer[(int)RenderLayer::Opaque].push_back(leftSphereRitem.get());
        mRitemLayer[(int)RenderLayer::Opaque].push_back(rightSphereRitem.get());

        mAllRitems.push_back(std::move(leftCylRitem));
        mAllRitems.push_back(std::move(rightCylRitem));
        mAllRitems.push_back(std::move(leftSphereRitem));
        mAllRitems.push_back(std::move(rightSphereRitem));
    }

    for(UINT i = 0; i < mSkinnedMats.size(); ++i)
    {
        std::string submeshName = "sm_" + std::to_string(i);

        auto ritem = std::make_unique<RenderItem>();

        // Reflect to change coordinate system from the RHS the data was exported out as.
        XMMATRIX modelScale = XMMatrixScaling(0.05f, 0.05f, -0.05f);
        XMMATRIX modelRot = XMMatrixRotationY(MathHelper::Pi);
        XMMATRIX modelOffset = XMMatrixTranslation(0.0f, 0.0f, -5.0f);
        XMStoreFloat4x4(&ritem->World, modelScale*modelRot*modelOffset);

        ritem->TexTransform = MathHelper::Identity4x4();
        ritem->ObjCBIndex = objCBIndex++;
        ritem->Mat = mMaterials[mSkinnedMats[i].Name].get();
        ritem->Geo = mGeometries[mSkinnedModelFilename].get();
        ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        ritem->IndexCount = ritem->Geo->DrawArgs[submeshName].IndexCount;
        ritem->StartIndexLocation = ritem->Geo->DrawArgs[submeshName].StartIndexLocation;
        ritem->BaseVertexLocation = ritem->Geo->DrawArgs[submeshName].BaseVertexLocation;

        // All render items for this solider.m3d instance share
        // the same skinned model instance.
        ritem->SkinnedCBIndex = 0;
        ritem->SkinnedModelInst = mSkinnedModelInst.get();

        mRitemLayer[(int)RenderLayer::SkinnedOpaque].push_back(ritem.get());
        mAllRitems.push_back(std::move(ritem));
    }

    */
    

}

void SkinnedMeshApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    // For each render item...
    for(size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];
        ri->Render(cmdList);
    }
}

void SkinnedMeshApp::DrawSceneToShadowMap()
{
    mCommandList->RSSetViewports(1, &mShadowMap->Viewport());
    mCommandList->RSSetScissorRects(1, &mShadowMap->ScissorRect());

    // Change to DEPTH_WRITE.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
        D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearDepthStencilView(mShadowMap->Dsv(), 
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->Dsv());

    // Bind the pass constant buffer for the shadow map pass.
    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
    auto passCB = mCurrFrameResource->PassCB->Resource();
    
    D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1*passCBByteSize;
    mCommandList->SetGraphicsRootConstantBufferView(2, mShadowMap->GetPassCBAdress());

    mCommandList->SetPipelineState(mPSOs["shadow_opaque"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

    mCommandList->SetPipelineState(mPSOs["skinnedShadow_opaque"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::SkinnedOpaque]);

    // Change back to GENERIC_READ so we can read the texture in a shader.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}
 
void SkinnedMeshApp::DrawNormalsAndDepth()
{
	mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

	auto normalMap = mSsao->NormalMap();
	auto normalMapRtv = mSsao->NormalMapRtv();
	
    // Change to RENDER_TARGET.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
        D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the screen normal map and depth buffer.
	float clearValue[] = {0.0f, 0.0f, 1.0f, 0.0f};
    mCommandList->ClearRenderTargetView(normalMapRtv, clearValue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &normalMapRtv, true, &DepthStencilView());

    // Bind the constant buffer for this pass.
    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    mCommandList->SetPipelineState(mPSOs["drawNormals"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

    mCommandList->SetPipelineState(mPSOs["skinnedDrawNormals"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::SkinnedOpaque]);

    // Change back to GENERIC_READ so we can read the texture in a shader.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

float SkinnedMeshApp::DeltaTime()
{
    return mTimer.DeltaTime();
}

void SkinnedMeshApp::AddInitFunc(void(*func)())
{
    initFunction.push_back(func);
}

void SkinnedMeshApp::AddUpdateFunc(void(*func)())
{
    updateFunction.push_back(func);
}

void SkinnedMeshApp::AddDrawFunc(void(*func)())
{
    drawFunction.push_back(func);
}
void SkinnedMeshApp::AddDestoryFunc(void(*func)())
{
    destoryFunction.push_back(func);
}
void SkinnedMeshApp::AddMsgProcFunc(LRESULT(*func)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam))
{
    msgProc.push_back(func);
}
HWND SkinnedMeshApp::GetMainWnd()
{
    return mhMainWnd;
}
ComPtr<ID3D12Device> SkinnedMeshApp::GetDevice()
{
    return md3dDevice;
}
ComPtr<ID3D12DescriptorHeap> SkinnedMeshApp::GetSrvHeap()
{
    return mSrvHeap;
}
ComPtr<ID3D12GraphicsCommandList> SkinnedMeshApp::GetCommandList()
{
    return mCommandList;
}
SkinnedMeshApp::~SkinnedMeshApp()
{
    for (size_t i = 0; i < destoryFunction.size(); i++)
    {
        destoryFunction[i]();
    }
    if (md3dDevice != nullptr)
        FlushCommandQueue();
}
LRESULT SkinnedMeshApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    for (size_t i = 0; i < msgProc.size(); i++)
    {
        msgProc[i](hwnd, msg, wParam, lParam);
    }
   return D3DApp::MsgProc(hwnd, msg, wParam, lParam);
}
vector<RenderItem*> SkinnedMeshApp::GetRenderItem()
{
    return mAllRitems;
}
void SkinnedMeshApp::InitScript()
{
    for (size_t i = 0; i < initFunction.size(); i++)
    {
        initFunction[i]();
    }
}
CD3DX12_CPU_DESCRIPTOR_HANDLE SkinnedMeshApp::GetCpuSrv(int index)const
{
    auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    srv.Offset(index, mCbvSrvUavDescriptorSize);
    return srv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE SkinnedMeshApp::GetGpuSrv(int index)const
{
    auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    srv.Offset(index, mCbvSrvUavDescriptorSize);
    return srv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE SkinnedMeshApp::GetDsv(int index)const
{
    auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
    dsv.Offset(index, mDsvDescriptorSize);
    return dsv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE SkinnedMeshApp::GetRtv(int index)const
{
    auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtv.Offset(index, mRtvDescriptorSize);
    return rtv;
}
