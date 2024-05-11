#include"renderItem.h"

#include "Common/GeometryGenerator.h"
Assimp::Importer g_aiImporter;
GeometryGenerator geoGen;
const XMMATRIX& MXEqual(XMMATRIX& mxDX, const aiMatrix4x4& mxAI)
{
	mxDX = XMMatrixSet(mxAI.a1, mxAI.a2, mxAI.a3, mxAI.a4,
		mxAI.b1, mxAI.b2, mxAI.b3, mxAI.b4,
		mxAI.c1, mxAI.c2, mxAI.c3, mxAI.c4,
		mxAI.d1, mxAI.d2, mxAI.d3, mxAI.d4);
	return mxDX;
}
const void copyXMFloat4x4(XMFLOAT4X4* res, aiMatrix4x4 source)
{
	res = &XMFLOAT4X4(source.a1, source.a2, source.a3, source.a4,
		source.b1, source.b2, source.b3, source.b4,
		source.c1, source.c2, source.c3, source.c4,
		source.d1, source.d2, source.d3, source.d4
		);
}
const void XMFloat4x4Transpose(XMFLOAT4X4* res, XMFLOAT4X4* r)
{
	res->_11 = r->_11;
	res->_12 = r->_21;
	res->_13 = r->_31;
	res->_14 = r->_41;
	res->_21 = r->_12;
	res->_22 = r->_22;
	res->_23 = r->_32;
	res->_24 = r->_42;
	res->_31 = r->_13;
	res->_32 = r->_23;
	res->_33 = r->_33;
	res->_34 = r->_43;
	res->_41 = r->_14;
	res->_42 = r->_24;
	res->_43 = r->_34;
	res->_44 = r->_44;
}
bool RenderItem::initAsSkinModel(string pszFileName, UINT nFlags)
{
	boneStateNow = new AllBones;
	Geo = new MeshGeometry;
	m_strFileName = pszFileName;
	m_nCurrentAnimIndex = 0;
	m_paiModel = g_aiImporter.ReadFile(pszFileName, nFlags);
	if (nullptr == m_paiModel)
	{
		return FALSE;
	}



	copyXMFloat4x4(&m_ModelTranformation, m_paiModel->mRootNode->mTransformation);

	UINT boneIndex = 0;
	for (size_t i = 0; i < m_paiModel->mNumMaterials; i++)
	{


		aiString aistrPath;
		aiMaterial* pMaterial = m_paiModel->mMaterials[i];
		if (!MaterialMgr::GetInstance().haveMaterial(pMaterial->GetName().data))
		{
			wstring diffuseName = L"";
			wstring normalName = L"";
			if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			{
				if (pMaterial->GetTexture(aiTextureType_DIFFUSE
					, 0, &aistrPath, nullptr, nullptr, nullptr, nullptr, nullptr)
					== AI_SUCCESS)
				{
					string path = m_strFileName.substr(0, m_strFileName.find_last_of('\\') + 1) + aistrPath.C_Str();
					path = path.substr(0, path.find_last_of('.')) + ".dds";
					wstring ws(path.begin(), path.end());
					diffuseName = ws;
				}
			}
			if (pMaterial->GetTextureCount(aiTextureType_NORMALS) > 0)
			{
				if (pMaterial->GetTexture(aiTextureType_NORMALS
					, 0, &aistrPath, nullptr, nullptr, nullptr, nullptr, nullptr)
					== AI_SUCCESS)
				{
					string path = m_strFileName.substr(0, m_strFileName.find_last_of('\\') + 1) + aistrPath.C_Str();
					path = path.substr(0, path.find_last_of('.')) + ".dds";
					wstring ws(path.begin(), path.end());
					normalName = ws;
				}
			}
			auto m = XMFLOAT4(1,1,1,1);
			auto t = XMFLOAT3(0,0,0);
			local2worldMaterialIndex[i]=MaterialMgr::GetInstance().createMaterial(
																					pMaterial->GetName().data,
																					diffuseName, normalName,
																					m,t,0.1);
		}
		else
		{
			local2worldMaterialIndex[i] = MaterialMgr::GetInstance().GetMaterialIndex(pMaterial->GetName().data);
		}
	}

	for (size_t i = 0; i < m_paiModel->mNumMeshes; i++)
	{
		SubmeshGeometry meshInfo;
		auto mesh_t = m_paiModel->mMeshes[i];
		meshInfo.MaterialIndex = local2worldMaterialIndex[mesh_t->mMaterialIndex];
		meshInfo.StartIndexLocation = FaceIndex.size();
		meshInfo.BaseVertexLocation = Vertices.size();
		meshInfo.IndexCount = 0;
		XMFLOAT3 mMax(mesh_t->mAABB.mMax.x, mesh_t->mAABB.mMax.y, mesh_t->mAABB.mMax.z);
		XMVECTOR mMax_v = XMLoadFloat3(&mMax);

		XMFLOAT3 mMin(mesh_t->mAABB.mMin.x, mesh_t->mAABB.mMin.y, mesh_t->mAABB.mMin.z);
		XMVECTOR mMin_v = XMLoadFloat3(&mMin);
		BoundingBox bound;
		XMStoreFloat3(&bound.Center, 0.5f * (mMax_v + mMin_v));
		XMStoreFloat3(&bound.Extents, 0.5f * (mMax_v - mMin_v));
		meshInfo.Bounds = bound;

		
		vectorBoneWeights.resize(mesh_t->mNumVertices + meshInfo.BaseVertexLocation);

		if (mesh_t->HasPositions())
		{

			for (size_t j = 0; j < mesh_t->mNumVertices; j++)
			{
				XMFLOAT4 ver(mesh_t->mVertices[j].x, mesh_t->mVertices[j].y, mesh_t->mVertices[j].z, 1.0f);
				Vertices.push_back(ver);
			}
			for (size_t j = 0; j < mesh_t->mNumFaces; j++)
			{
				for (size_t k = 0; k < 3; k++)
				{
					FaceIndex.push_back(mesh_t->mFaces[j].mIndices[k]);
					meshInfo.IndexCount++;
				}
			}
		}
		else
		{
			for (size_t j = 0; j < mesh_t->mNumVertices; j++)
			{
				XMFLOAT4 ver(0, 0, 0, 1.0f);
				Vertices.push_back(ver);
			}
		}
		if (mesh_t->HasNormals())
		{

			for (size_t j = 0; j < mesh_t->mNumVertices; j++)
			{
				XMFLOAT4 nor(mesh_t->mNormals[j].x, mesh_t->mNormals[j].y, mesh_t->mNormals[j].z, 1.0f);
				Normals.push_back(nor);
			}
		}
		else
		{
			for (size_t j = 0; j < mesh_t->mNumVertices; j++)
			{
				XMFLOAT4 ver(0, 0, 0, 0);
				Normals.push_back(ver);
			}
		}
		if (mesh_t->HasTangentsAndBitangents())
		{

			for (size_t j = 0; j < mesh_t->mNumVertices; j++)
			{
				XMFLOAT4 tan(mesh_t->mTangents[j].x, mesh_t->mTangents[j].y, mesh_t->mTangents[j].z, 1.0f);
				XMFLOAT4 Btan(mesh_t->mBitangents[j].x, mesh_t->mBitangents[j].y, mesh_t->mBitangents[j].z, 1.0f);
				Tangents.push_back(tan);
				Bitangents.push_back(Btan);
			}
		}
		else
		{
			for (size_t j = 0; j < mesh_t->mNumVertices; j++)
			{
				XMFLOAT4 ver(0, 0, 0, 0);
				Tangents.push_back(ver);
				Bitangents.push_back(ver);

			}
		}
		if (mesh_t->HasTextureCoords(0))
		{
			for (size_t j = 0; j < mesh_t->mNumVertices; j++)
			{
				XMFLOAT2 TCoord(mesh_t->mTextureCoords[0][j].x, mesh_t->mTextureCoords[0][j].y);
				TextureCoords.push_back(TCoord);
			}
		}
		else
		{
			for (size_t j = 0; j < mesh_t->mNumVertices; j++)
			{
				TextureCoords.push_back( XMFLOAT2(0.0f, 0.0f));
			}
		}

		for (size_t k = 0; k < mesh_t->mNumBones; k++)
		{


			aiBone* pBone = mesh_t->mBones[k];
			if (m_mapName2Bone.end() == m_mapName2Bone.find(pBone->mName.data))
			{
				m_mapName2Bone[pBone->mName.data] = boneIndex;
				Bonedata* temp = new Bonedata;
				temp->offsetMatrix = XMMatrixTranspose(MXEqual(temp->offsetMatrix, pBone->mOffsetMatrix));
				BoneData.push_back(temp);
				XMMATRIX t = XMMatrixSet(pBone->mOffsetMatrix.a1, pBone->mOffsetMatrix.b1, pBone->mOffsetMatrix.c1, pBone->mOffsetMatrix.d1,
					pBone->mOffsetMatrix.a2, pBone->mOffsetMatrix.b2, pBone->mOffsetMatrix.c2, pBone->mOffsetMatrix.d2,
					pBone->mOffsetMatrix.a3, pBone->mOffsetMatrix.b3, pBone->mOffsetMatrix.c3, pBone->mOffsetMatrix.d3,
					pBone->mOffsetMatrix.a4, pBone->mOffsetMatrix.b4, pBone->mOffsetMatrix.c4, pBone->mOffsetMatrix.d4
				);
				boneIndex++;
			}
			for (UINT k = 0; k < pBone->mNumWeights; k++)
			{
				UINT t = m_mapName2Bone[pBone->mName.data];
				float m = pBone->mWeights[k].mWeight;
				vectorBoneWeights[pBone->mWeights[k].mVertexId+ meshInfo.BaseVertexLocation].AddBoneData(m_mapName2Bone[pBone->mName.data], pBone->mWeights[k].mWeight);
			}
			if (pBone->mName.length > 0)
			{
				m_mapNodeName2BoneIndex[pBone->mName.data] = m_mapName2Bone.find(pBone->mName.data)->second;
			}
		}

		Geo->DrawArgs[to_string(i)+mesh_t->mName.data] = meshInfo;
	}


	if (m_paiModel->HasAnimations())
	{

		for (size_t i = 0; i < m_paiModel->mNumAnimations; i++)
		{
			aiAnimation* tempAnim = m_paiModel->mAnimations[i];
			AnimData resAnime;
			resAnime.mDuration = tempAnim->mDuration;
			resAnime.mTicksPerSecond = tempAnim->mTicksPerSecond;
			for (size_t j = 0; j < tempAnim->mNumChannels; j++)
			{
				aiNodeAnim* tempNodeAnim = tempAnim->mChannels[j];
				if (resAnime.BoneAnimChannel.find(tempNodeAnim->mNodeName.data) != resAnime.BoneAnimChannel.end())
				{
					NodeAnim nodeanim;
					nodeanim.Bonename = tempNodeAnim->mNodeName.data;
					nodeanim.mPositionKeys.resize(tempNodeAnim->mNumPositionKeys);
					nodeanim.mRotationKeys.resize(tempNodeAnim->mNumRotationKeys);
					nodeanim.mScalingKeys.resize(tempNodeAnim->mNumScalingKeys);
					resAnime.BoneAnimChannel[tempNodeAnim->mNodeName.data] = nodeanim;
				}
				auto nodeTemp = &resAnime.BoneAnimChannel[tempNodeAnim->mNodeName.data];
				for (size_t k = 0; k < tempNodeAnim->mNumPositionKeys; k++)
				{
					VectorKey tempPos;
					tempPos.mTime = tempNodeAnim->mPositionKeys[k].mTime;
					XMFLOAT3 t(tempNodeAnim->mPositionKeys[k].mValue.x, tempNodeAnim->mPositionKeys[k].mValue.y, tempNodeAnim->mPositionKeys[k].mValue.z);
					tempPos.mValue = t;
					nodeTemp->mPositionKeys.push_back(tempPos);
				}
				for (size_t k = 0; k < tempNodeAnim->mNumRotationKeys; k++)
				{
					QuaternionKey tempRot;
					tempRot.mTime = tempNodeAnim->mPositionKeys[k].mTime;
					XMFLOAT4 t(tempNodeAnim->mRotationKeys[k].mValue.x, tempNodeAnim->mRotationKeys[k].mValue.y, tempNodeAnim->mRotationKeys[k].mValue.z, tempNodeAnim->mRotationKeys[k].mValue.w);
					tempRot.mValue = t;
					nodeTemp->mRotationKeys.push_back(tempRot);
				}
				for (size_t k = 0; k < tempNodeAnim->mNumScalingKeys; k++)
				{
					VectorKey tempscale;
					tempscale.mTime = tempNodeAnim->mScalingKeys[k].mTime;
					XMFLOAT3 t(tempNodeAnim->mScalingKeys[k].mValue.x, tempNodeAnim->mScalingKeys[k].mValue.y, tempNodeAnim->mScalingKeys[k].mValue.z);
					tempscale.mValue = t;
					nodeTemp->mPositionKeys.push_back(tempscale);
				}

			}
			boneAnimationData[tempAnim->mName.data] = resAnime;
		}
	}
	return true;
}
bool RenderItem::initAsBox(float width, float height, float depth, UINT numSubdivisions)
{
	m_strFileName = "Box";
	IsBaseObj = true;
	GeometryGenerator::MeshData box = geoGen.CreateBox(width,height,depth,numSubdivisions);
	SubmeshGeometry* boxSubmesh=new SubmeshGeometry;
	boxSubmesh->IndexCount = (UINT)box.Indices32.size();
	boxSubmesh->StartIndexLocation = 0;
	boxSubmesh->BaseVertexLocation = 0;
	for (size_t i = 0; i < box.Indices32.size(); i++)
	{
		FaceIndex.push_back(box.Indices32[i]);
	}
	for (size_t i = 0; i < box.Vertices.size(); i++)
	{
		Vertices.push_back(XMFLOAT4( box.Vertices[i].Position.x, box.Vertices[i].Position.y, box.Vertices[i].Position.z, 1.0f));
		Normals.push_back(XMFLOAT4(box.Vertices[i].Normal.x, box.Vertices[i].Normal.y, box.Vertices[i].Normal.z, 0));
		TextureCoords.push_back(box.Vertices[i].TexC);
		Tangents.push_back(XMFLOAT4(box.Vertices[i].TangentU.x, box.Vertices[i].TangentU.y, box.Vertices[i].TangentU.z,0));
	}
	MeshGeometry* res = new MeshGeometry;
	res->DrawArgs["box"] = *boxSubmesh;
	Geo = res;
	return true;
}
bool RenderItem::initAsGrid(float width, float depth, UINT m, UINT n)
{
	m_strFileName = "Grid";
	IsBaseObj = true;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(width,depth,m,n);
	SubmeshGeometry* boxSubmesh = new SubmeshGeometry;
	boxSubmesh->IndexCount = (UINT)grid.Indices32.size();
	boxSubmesh->StartIndexLocation = 0;
	boxSubmesh->BaseVertexLocation = 0;
	for (size_t i = 0; i < grid.Indices32.size(); i++)
	{
		FaceIndex.push_back(grid.Indices32[i]);
	}
	for (size_t i = 0; i < grid.Vertices.size(); i++)
	{
		Vertices.push_back(XMFLOAT4(grid.Vertices[i].Position.x, grid.Vertices[i].Position.y, grid.Vertices[i].Position.z, 1.0f));
		Normals.push_back(XMFLOAT4(grid.Vertices[i].Normal.x, grid.Vertices[i].Normal.y, grid.Vertices[i].Normal.z, 0));
		TextureCoords.push_back(grid.Vertices[i].TexC);
		Tangents.push_back(XMFLOAT4(grid.Vertices[i].TangentU.x, grid.Vertices[i].TangentU.y, grid.Vertices[i].TangentU.z, 0));
	}
	MeshGeometry* res = new MeshGeometry;
	res->DrawArgs["Grid"] = *boxSubmesh;
	Geo = res;
	return true;
}
bool RenderItem::initAsSphere(float radius, UINT sliceCount, UINT stackCount)
{
	m_strFileName = "Sphere";
	IsBaseObj = true;
	GeometryGenerator::MeshData Sphere = geoGen.CreateSphere(radius,sliceCount,stackCount);
	SubmeshGeometry* sphereSubmesh = new SubmeshGeometry;
	sphereSubmesh->IndexCount = (UINT)Sphere.Indices32.size();
	sphereSubmesh->StartIndexLocation = 0;
	sphereSubmesh->BaseVertexLocation = 0;
	for (size_t i = 0; i < Sphere.Indices32.size(); i++)
	{
		FaceIndex.push_back(Sphere.Indices32[i]);
	}
	for (size_t i = 0; i < Sphere.Vertices.size(); i++)
	{
		Vertices.push_back(XMFLOAT4(Sphere.Vertices[i].Position.x, Sphere.Vertices[i].Position.y, Sphere.Vertices[i].Position.z, 1.0f));
		Normals.push_back(XMFLOAT4(Sphere.Vertices[i].Normal.x, Sphere.Vertices[i].Normal.y, Sphere.Vertices[i].Normal.z, 0));
		TextureCoords.push_back(Sphere.Vertices[i].TexC);
		Tangents.push_back(XMFLOAT4(Sphere.Vertices[i].TangentU.x, Sphere.Vertices[i].TangentU.y, Sphere.Vertices[i].TangentU.z, 0));
	}
	MeshGeometry* res = new MeshGeometry;
	res->DrawArgs["sphere"] = *sphereSubmesh;
	Geo = res;
	return true;
}
bool RenderItem::initAsCylinder(float bottomRadius, float topRadius, float height, UINT sliceCount, UINT stackCount)
{
	m_strFileName = "Cylinder";
	IsBaseObj = true;
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(bottomRadius,topRadius,height,sliceCount,stackCount);
	SubmeshGeometry* boxSubmesh = new SubmeshGeometry;
	boxSubmesh->IndexCount = (UINT)cylinder.Indices32.size();
	boxSubmesh->StartIndexLocation = 0;
	boxSubmesh->BaseVertexLocation = 0;
	for (size_t i = 0; i < cylinder.Indices32.size(); i++)
	{
		FaceIndex.push_back(cylinder.Indices32[i]);
	}
	for (size_t i = 0; i < cylinder.Vertices.size(); i++)
	{
		Vertices.push_back(XMFLOAT4(cylinder.Vertices[i].Position.x, cylinder.Vertices[i].Position.y, cylinder.Vertices[i].Position.z, 1.0f));
		Normals.push_back(XMFLOAT4(cylinder.Vertices[i].Normal.x, cylinder.Vertices[i].Normal.y, cylinder.Vertices[i].Normal.z, 0));
		TextureCoords.push_back(cylinder.Vertices[i].TexC);
		Tangents.push_back(XMFLOAT4(cylinder.Vertices[i].TangentU.x, cylinder.Vertices[i].TangentU.y, cylinder.Vertices[i].TangentU.z, 0));
	}
	MeshGeometry* res = new MeshGeometry;
	res->DrawArgs["cylinder"] = *boxSubmesh;
	Geo = res;
	return true;
}
bool RenderItem::initAsQuad(float x, float y, float w, float h, float depth)
{
	m_strFileName = "Quad";
	IsBaseObj = true;
	GeometryGenerator::MeshData box = geoGen.CreateQuad(x,y,w,h,depth);
	SubmeshGeometry* boxSubmesh = new SubmeshGeometry;
	boxSubmesh->IndexCount = (UINT)box.Indices32.size();
	boxSubmesh->StartIndexLocation = 0;
	boxSubmesh->BaseVertexLocation = 0;
	for (size_t i = 0; i < box.Indices32.size(); i++)
	{
		FaceIndex.push_back(box.Indices32[i]);
	}
	for (size_t i = 0; i < box.Vertices.size(); i++)
	{
		Vertices.push_back(XMFLOAT4(box.Vertices[i].Position.x, box.Vertices[i].Position.y, box.Vertices[i].Position.z, 1.0f));
		Normals.push_back(XMFLOAT4(box.Vertices[i].Normal.x, box.Vertices[i].Normal.y, box.Vertices[i].Normal.z, 0));
		TextureCoords.push_back(box.Vertices[i].TexC);
		Tangents.push_back(XMFLOAT4(box.Vertices[i].TangentU.x, box.Vertices[i].TangentU.y, box.Vertices[i].TangentU.z, 0));
	}
	MeshGeometry* res = new MeshGeometry;
	res->DrawArgs["quad"] = *boxSubmesh;
	Geo = res;
	return true;
}

bool RenderItem::initScript()
{

	return true;
}

void RenderItem::SetMaterial(string materialName)
{
	if (IsBaseObj)
	{
		for (auto m:Geo->DrawArgs)
		{
			auto s= MaterialMgr::GetInstance().GetMaterialIndex(materialName);
			Geo->DrawArgs[m.first].MaterialIndex = MaterialMgr::GetInstance().GetMaterialIndex(materialName);
		}
	}
}
void RenderItem::SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY type)
{
	PrimitiveType = type;
}
void RenderItem::SetScale(XMFLOAT3 Scale)
{
	m_ModelTranformation._11 = Scale.x;
	m_ModelTranformation._22 = Scale.y;
	m_ModelTranformation._33 = Scale.z;
	NumFramesDirty++;
}

HRESULT RenderItem::UploadModel(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList)
{
	ObjectCB = make_unique<UploadBuffer<ObjectConstants>>(device, 1, true);
	ObjectConstants temp;
	temp.MaterialIndex = 0;
	temp.TexTransform =XMFLOAT4X4(1,0,0,0,
								  0,1,0,0,
								  0,0,1,0,
								  0,0,0,1);
	temp.World= XMFLOAT4X4(1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
	ObjectCB->CopyData(0,temp);
	if (!IsBaseObj)
	{
		skinCB = make_unique<UploadBuffer<AllBones>>(device, 1, true);
		AllBones t ;
		for (size_t i = 0; i < _countof(t.mxBones); i++)
		{
			t.mxBones[i] = XMMatrixIdentity();
		}
		skinCB->CopyData(0, t);
	}
		size_t offset = 0;

		size_t v_size = Vertices.size() * sizeof(XMFLOAT4);
		size_t n_size = Normals.size() * sizeof(XMFLOAT4);
		size_t t_size = TextureCoords.size() * sizeof(XMFLOAT2);
		size_t tan_size = Tangents.size() * sizeof(XMFLOAT4);
		size_t b_size = vectorBoneWeights.size() * sizeof(myVertexBone);
		size_t i_size = FaceIndex.size() * sizeof(UINT);
		size_t szAlign = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		size_t szVBBuffer = d3dUtil::CalcConstantBufferByteSize(v_size, szAlign) +
			d3dUtil::CalcConstantBufferByteSize(n_size, szAlign) +
			d3dUtil::CalcConstantBufferByteSize(t_size, szAlign) +
			d3dUtil::CalcConstantBufferByteSize(tan_size, szAlign) +
			d3dUtil::CalcConstantBufferByteSize(b_size, szAlign) +
			d3dUtil::CalcConstantBufferByteSize(i_size, szAlign) +
			szAlign - 1;


		auto verticesResource = CD3DX12_RESOURCE_DESC::Buffer(v_size);
		auto normalsResource = CD3DX12_RESOURCE_DESC::Buffer(n_size);
		auto texCoordsResource = CD3DX12_RESOURCE_DESC::Buffer(t_size);
		auto tanResource = CD3DX12_RESOURCE_DESC::Buffer(tan_size);
		auto boneWeightResource = CD3DX12_RESOURCE_DESC::Buffer(b_size);
		auto indicesResource = CD3DX12_RESOURCE_DESC::Buffer(i_size);


		D3D12_RESOURCE_DESC stBufferResSesc = {};
		stBufferResSesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		stBufferResSesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		stBufferResSesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		stBufferResSesc.Format = DXGI_FORMAT_UNKNOWN;
		stBufferResSesc.Width = 0;
		stBufferResSesc.Height = 1;
		stBufferResSesc.DepthOrArraySize = 1;
		stBufferResSesc.MipLevels = 1;
		stBufferResSesc.SampleDesc.Count = 1;
		stBufferResSesc.SampleDesc.Quality = 0;

		D3D12_HEAP_PROPERTIES stDefautHeapProps = {};
		stDefautHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		stDefautHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stDefautHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stDefautHeapProps.CreationNodeMask = 0;
		stDefautHeapProps.VisibleNodeMask = 0;

		D3D12_HEAP_DESC stDefaultHeapDesc = {};
		stDefaultHeapDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(szVBBuffer, szAlign);
		stDefaultHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		stDefaultHeapDesc.Alignment = szAlign;
		stDefaultHeapDesc.Properties = stDefautHeapProps;

		ComPtr<ID3D12Heap>					defaultHeap;
		ThrowIfFailed(device->CreateHeap(&stDefaultHeapDesc, IID_PPV_ARGS(&defaultHeap)));
		stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(v_size, szAlign);
		ThrowIfFailed(device->CreatePlacedResource(defaultHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&Geo->VerticesResource)));
		offset += stBufferResSesc.Width;
		stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(n_size, szAlign);
		ThrowIfFailed(device->CreatePlacedResource(defaultHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&Geo->NormalsResource)));
		offset += stBufferResSesc.Width;
		stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(tan_size, szAlign);
		ThrowIfFailed(device->CreatePlacedResource(defaultHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&Geo->TangentResource)));
		offset += stBufferResSesc.Width;
		stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(t_size, szAlign);
		ThrowIfFailed(device->CreatePlacedResource(defaultHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&Geo->TexCoordsResource)));
		offset += stBufferResSesc.Width;
		if (!IsBaseObj)
		{
			stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(b_size, szAlign);
			ThrowIfFailed(device->CreatePlacedResource(defaultHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&Geo->BoneWeightResource)));
			offset += stBufferResSesc.Width;
		}
		stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(i_size, szAlign);
		ThrowIfFailed(device->CreatePlacedResource(defaultHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&Geo->IndicesResource)));
		offset = 0;

		ComPtr<ID3D12Resource> verticesUpload;
		ComPtr<ID3D12Resource> normalUpload;
		ComPtr<ID3D12Resource> tangentUpload;
		ComPtr<ID3D12Resource> texCoordsUpload;
		ComPtr<ID3D12Resource> boneWeightUpload;
		ComPtr<ID3D12Resource> indicesUpload;


		D3D12_HEAP_PROPERTIES stUploadHeapProps = {};
		stUploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		stUploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		stUploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		stUploadHeapProps.CreationNodeMask = 0;
		stUploadHeapProps.VisibleNodeMask = 0;

		D3D12_HEAP_DESC stUploadHeapDesc = {  };
		stUploadHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		stUploadHeapDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(szVBBuffer, szAlign);
		stUploadHeapDesc.Alignment = 0;
		stUploadHeapDesc.Properties = stUploadHeapProps;
		ComPtr<ID3D12Heap>					uploadHeap;
		ThrowIfFailed(device->CreateHeap(&stUploadHeapDesc, IID_PPV_ARGS(&uploadHeap)));

		stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(v_size, szAlign);
		ThrowIfFailed(device->CreatePlacedResource(uploadHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&verticesUpload)));
		offset += stBufferResSesc.Width;
		stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(n_size, szAlign);
		ThrowIfFailed(device->CreatePlacedResource(uploadHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&normalUpload)));
		offset += stBufferResSesc.Width;
		stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(tan_size, szAlign);
		ThrowIfFailed(device->CreatePlacedResource(uploadHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&tangentUpload)));
		offset += stBufferResSesc.Width;
		stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(t_size, szAlign);
		ThrowIfFailed(device->CreatePlacedResource(uploadHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&texCoordsUpload)));
		offset += stBufferResSesc.Width;
		if (!IsBaseObj)
		{
			stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(b_size, szAlign);
			ThrowIfFailed(device->CreatePlacedResource(uploadHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&boneWeightUpload)));
			offset += stBufferResSesc.Width;

		}
		stBufferResSesc.Width = d3dUtil::CalcConstantBufferByteSize(i_size, szAlign);
		ThrowIfFailed(device->CreatePlacedResource(uploadHeap.Get(), offset, &stBufferResSesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indicesUpload)));
		offset = 0;





		BYTE* data_temp;


		ThrowIfFailed(verticesUpload->Map(0, nullptr, reinterpret_cast<void**>(&data_temp)));
		memcpy(data_temp, Vertices.data(), verticesResource.Width);
		verticesUpload->Unmap(0, nullptr);
		data_temp = nullptr;

		ThrowIfFailed(normalUpload->Map(0, nullptr, reinterpret_cast<void**>(&data_temp)));
		memcpy(data_temp, Normals.data(), normalsResource.Width);
		normalUpload->Unmap(0, nullptr);
		data_temp = nullptr;

		ThrowIfFailed(tangentUpload->Map(0, nullptr, reinterpret_cast<void**>(&data_temp)));
		memcpy(data_temp, Tangents.data(), tanResource.Width);
		tangentUpload->Unmap(0, nullptr);
		data_temp = nullptr;

		ThrowIfFailed(texCoordsUpload->Map(0, nullptr, reinterpret_cast<void**>(&data_temp)));
		memcpy(data_temp, TextureCoords.data(), texCoordsResource.Width);
		texCoordsUpload->Unmap(0, nullptr);
		data_temp = nullptr;
		if (!IsBaseObj)
		{
			ThrowIfFailed(boneWeightUpload->Map(0, nullptr, reinterpret_cast<void**>(&data_temp)));
			memcpy(data_temp, vectorBoneWeights.data(), boneWeightResource.Width);
			boneWeightUpload->Unmap(0, nullptr);
			data_temp = nullptr;
		}


		ThrowIfFailed(indicesUpload->Map(0, nullptr, reinterpret_cast<void**>(&data_temp)));
		memcpy(data_temp, FaceIndex.data(), indicesResource.Width);
		indicesUpload->Unmap(0, nullptr);
		data_temp = nullptr;

		cmdList->Close();
		cmdList->Reset(cmdAlloc, nullptr);
		if (IsBaseObj)
		{
			D3D12_RESOURCE_BARRIER copyBarry[5];


			cmdList->CopyResource(Geo->VerticesResource.Get(), verticesUpload.Get());
			copyBarry[0] = CD3DX12_RESOURCE_BARRIER::Transition(Geo->VerticesResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdList->CopyResource(Geo->NormalsResource.Get(), normalUpload.Get());
			copyBarry[1] = CD3DX12_RESOURCE_BARRIER::Transition(Geo->NormalsResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdList->CopyResource(Geo->TexCoordsResource.Get(), texCoordsUpload.Get());
			copyBarry[2] = CD3DX12_RESOURCE_BARRIER::Transition(Geo->TexCoordsResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdList->CopyResource(Geo->TangentResource.Get(), tangentUpload.Get());
			copyBarry[3] = CD3DX12_RESOURCE_BARRIER::Transition(Geo->TangentResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdList->CopyResource(Geo->IndicesResource.Get(), indicesUpload.Get());
			copyBarry[4] = CD3DX12_RESOURCE_BARRIER::Transition(Geo->IndicesResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			cmdList->ResourceBarrier(5, copyBarry);
		}
		else
		{
			D3D12_RESOURCE_BARRIER copyBarry[6];


			cmdList->CopyResource(Geo->VerticesResource.Get(), verticesUpload.Get());
			copyBarry[0] = CD3DX12_RESOURCE_BARRIER::Transition(Geo->VerticesResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdList->CopyResource(Geo->NormalsResource.Get(), normalUpload.Get());
			copyBarry[1] = CD3DX12_RESOURCE_BARRIER::Transition(Geo->NormalsResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdList->CopyResource(Geo->TexCoordsResource.Get(), texCoordsUpload.Get());
			copyBarry[2] = CD3DX12_RESOURCE_BARRIER::Transition(Geo->TexCoordsResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdList->CopyResource(Geo->TangentResource.Get(), tangentUpload.Get());
			copyBarry[3] = CD3DX12_RESOURCE_BARRIER::Transition(Geo->TangentResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdList->CopyResource(Geo->BoneWeightResource.Get(), boneWeightUpload.Get());
			copyBarry[4] = CD3DX12_RESOURCE_BARRIER::Transition(Geo->BoneWeightResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdList->CopyResource(Geo->IndicesResource.Get(), indicesUpload.Get());
			copyBarry[5] = CD3DX12_RESOURCE_BARRIER::Transition(Geo->IndicesResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			cmdList->ResourceBarrier(6, copyBarry);
		}



		Geo->mIBV.BufferLocation = Geo->IndicesResource->GetGPUVirtualAddress();
		Geo->mIBV.Format = (sizeof(FaceIndex[0]) > 2) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		Geo->mIBV.SizeInBytes = (UINT)i_size;


		Geo->mVBV[0].BufferLocation = Geo->VerticesResource->GetGPUVirtualAddress();
		Geo->mVBV[0].SizeInBytes = (UINT)v_size;
		Geo->mVBV[0].StrideInBytes = sizeof(Vertices[0]);

		Geo->mVBV[1].BufferLocation = Geo->NormalsResource->GetGPUVirtualAddress();
		Geo->mVBV[1].SizeInBytes = (UINT)n_size;
		Geo->mVBV[1].StrideInBytes = sizeof(Normals[0]);

		Geo->mVBV[2].BufferLocation = Geo->TexCoordsResource->GetGPUVirtualAddress();
		Geo->mVBV[2].SizeInBytes = (UINT)t_size;
		Geo->mVBV[2].StrideInBytes = sizeof(TextureCoords[0]);

		Geo->mVBV[3].BufferLocation = Geo->TangentResource->GetGPUVirtualAddress();
		Geo->mVBV[3].SizeInBytes = (UINT)tan_size;
		Geo->mVBV[3].StrideInBytes = sizeof(Tangents[0]);
		if (!IsBaseObj)
		{
			Geo->mVBV[4].BufferLocation = Geo->BoneWeightResource->GetGPUVirtualAddress();
			Geo->mVBV[4].SizeInBytes = (UINT)b_size;
			Geo->mVBV[4].StrideInBytes = sizeof(vectorBoneWeights[0]);
		}

		ThrowIfFailed(cmdList->Close());

		ID3D12CommandList* ppCommandLists[] = { cmdList };
		cmdQueue->ExecuteCommandLists(1, ppCommandLists);

		ComPtr<ID3D12Fence> fence;
		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

		cmdQueue->Signal(fence.Get(), 1);

		if (fence->GetCompletedValue() != 1)
		{
			HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			fence->SetEventOnCompletion(1, event);

			WaitForSingleObjectEx(event, INFINITE, false);
			CloseHandle(event);
		}

		cmdList->Reset(cmdAlloc, nullptr);

	return S_OK;
}
void RenderItem::CalculateNodeAnim(aiNode* root, XMMATRIX parentTransform, AnimData* anime, int* i)
{
	if (root->mName.length > 0)
	{
		NodeAnim tempNodeAnim = anime->BoneAnimChannel[root->mName.data];
		XMMATRIX pos_matrix = XMMatrixIdentity();
		XMMATRIX rota_matrix = XMMatrixIdentity();
		XMMATRIX scale_matrix = XMMatrixIdentity();
		float currentPart = 0;
		if (tempNodeAnim.mPositionKeys.size() > 0)
		{
			while (tempNodeAnim.mPositionKeys.size() > nodeQueueReadNum[*i][0] && tempNodeAnim.mPositionKeys[nodeQueueReadNum[*i][0]].mTime <= m_nCurrentAnimIndex)
			{
				nodeQueueReadNum[*i][0]++;
			}
			XMVECTOR pos_ever = XMVectorSet(tempNodeAnim.mPositionKeys[nodeQueueReadNum[*i][0] - 1].mValue.x, tempNodeAnim.mPositionKeys[nodeQueueReadNum[*i][0] - 1].mValue.y, tempNodeAnim.mPositionKeys[nodeQueueReadNum[*i][0] - 1].mValue.z, 0);
			XMVECTOR pos_future = XMVectorSet(tempNodeAnim.mPositionKeys[nodeQueueReadNum[*i][0]].mValue.x, tempNodeAnim.mPositionKeys[nodeQueueReadNum[*i][0]].mValue.y, tempNodeAnim.mPositionKeys[nodeQueueReadNum[*i][0]].mValue.z, 0);
			currentPart = (m_nCurrentAnimIndex - tempNodeAnim.mPositionKeys[nodeQueueReadNum[*i][0] - 1].mTime) / (tempNodeAnim.mPositionKeys[nodeQueueReadNum[*i][0]].mTime - tempNodeAnim.mPositionKeys[nodeQueueReadNum[*i][0] - 1].mTime);
			XMVECTOR pos = XMVectorLerp(pos_ever, pos_future, currentPart);
			pos_matrix = XMMatrixTranslationFromVector(pos);
		}
		if (tempNodeAnim.mRotationKeys.size() > 0)
		{
			while (tempNodeAnim.mRotationKeys.size() > nodeQueueReadNum[*i][1] && tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1]].mTime <= m_nCurrentAnimIndex)
			{
				nodeQueueReadNum[*i][1]++;
			}
			XMVECTOR rota_ever = XMVectorSet(tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1] - 1].mValue.x, tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1] - 1].mValue.y, tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1] - 1].mValue.z, tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1] - 1].mValue.w);
			XMVECTOR rota_future = XMVectorSet(tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1]].mValue.x, tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1]].mValue.y, tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1]].mValue.z, tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1]].mValue.w);
			currentPart = (m_nCurrentAnimIndex - tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1] - 1].mTime) / (tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1]].mTime - tempNodeAnim.mRotationKeys[nodeQueueReadNum[*i][1] - 1].mTime);
			XMVECTOR rota = XMVectorLerp(rota_ever, rota_future, currentPart);
			rota_matrix = XMMatrixRotationQuaternion(rota);
		}
		if (tempNodeAnim.mScalingKeys.size() > 0)
		{
			while (tempNodeAnim.mScalingKeys.size() > nodeQueueReadNum[*i][2] && tempNodeAnim.mScalingKeys[nodeQueueReadNum[*i][2]].mTime <= m_nCurrentAnimIndex)
			{
				nodeQueueReadNum[*i][2]++;
			}
			XMVECTOR scale_ever = XMVectorSet(tempNodeAnim.mScalingKeys[nodeQueueReadNum[*i][2] - 1].mValue.x, tempNodeAnim.mScalingKeys[nodeQueueReadNum[*i][2] - 1].mValue.y, tempNodeAnim.mScalingKeys[nodeQueueReadNum[*i][2] - 1].mValue.z, 0);
			XMVECTOR scale_future = XMVectorSet(tempNodeAnim.mScalingKeys[nodeQueueReadNum[*i][2]].mValue.x, tempNodeAnim.mScalingKeys[nodeQueueReadNum[*i][2]].mValue.y, tempNodeAnim.mScalingKeys[nodeQueueReadNum[*i][2]].mValue.z, 0);
			currentPart = (m_nCurrentAnimIndex - tempNodeAnim.mScalingKeys[nodeQueueReadNum[*i][2] - 1].mTime) / (tempNodeAnim.mScalingKeys[nodeQueueReadNum[*i][2]].mTime - tempNodeAnim.mScalingKeys[nodeQueueReadNum[*i][2] - 1].mTime);
			XMVECTOR scale = XMVectorLerp(scale_ever, scale_future, currentPart);
			scale_matrix = XMMatrixScalingFromVector(scale);
		}

		XMMATRIX res_mareix = scale_matrix * rota_matrix * pos_matrix * parentTransform;
		XMMATRIX modelTransform=XMMatrixSet(m_ModelTranformation._11, m_ModelTranformation._12, m_ModelTranformation._13, m_ModelTranformation._14, 
			m_ModelTranformation._21, m_ModelTranformation._22, m_ModelTranformation._23, m_ModelTranformation._24,
			m_ModelTranformation._31, m_ModelTranformation._32, m_ModelTranformation._33, m_ModelTranformation._34,
			m_ModelTranformation._41, m_ModelTranformation._42, m_ModelTranformation._43, m_ModelTranformation._44 );
		XMMATRIX end = BoneData[m_mapNodeName2BoneIndex[root->mName.C_Str()]]->offsetMatrix * res_mareix * modelTransform;
		(*i)++;
		string ss = root->mName.data;
		BoneData[m_mapNodeName2BoneIndex[root->mName.C_Str()]]->m_mxFinalTransformation = end;
		for (size_t j = 0; j < root->mNumChildren; j++)
		{
			CalculateNodeAnim(root->mChildren[j], res_mareix, anime, i);
		}
	}
	else
	{
		for (size_t j = 0; j < root->mNumChildren; j++)
		{
			CalculateNodeAnim(root->mChildren[j], parentTransform, anime, i);
		}
	}


}
void RenderItem::BoneAnimePlay(int animIndex, double frameDeltesecond)
{
	if (IsBaseObj)
	{
		return;
	}
	string animName = m_paiModel->mAnimations[animIndex]->mName.data;
	double ticksPerSecond = 0;
	if (boneAnimationData.find(animName)->second.mTicksPerSecond != 0)
	{
		ticksPerSecond = boneAnimationData.find(animName)->second.mTicksPerSecond;
	}
	else
	{
		ticksPerSecond = 2500;
	}
	m_nCurrentAnimIndex += frameDeltesecond * ticksPerSecond;
	if (m_nCurrentAnimIndex >= boneAnimationData.find(animName)->second.mDuration)
	{
		m_nCurrentAnimIndex = 0;
		for (size_t i = 0; i < 256; i++)
		{
			for (size_t j = 0; j < 3; j++)
			{
				nodeQueueReadNum[i][j] = 0;
			}
		}
	}
	AnimData tempAnime = boneAnimationData[animName];
	int nodeIndexTemp = 0;
	XMMATRIX identy = XMMatrixIdentity();
	XMMATRIX modelTransform = XMMatrixSet(m_ModelTranformation._11, m_ModelTranformation._12, m_ModelTranformation._13, m_ModelTranformation._14,
		m_ModelTranformation._21, m_ModelTranformation._22, m_ModelTranformation._23, m_ModelTranformation._24,
		m_ModelTranformation._31, m_ModelTranformation._32, m_ModelTranformation._33, m_ModelTranformation._34,
		m_ModelTranformation._41, m_ModelTranformation._42, m_ModelTranformation._43, m_ModelTranformation._44);
	CalculateNodeAnim(m_paiModel->mRootNode, modelTransform, &boneAnimationData[animName], &nodeIndexTemp);
	for (size_t i = 0; i < BoneData.size(); i++)
	{
		boneStateNow->mxBones[i] = XMMatrixTranspose(BoneData[i]->m_mxFinalTransformation);
	}
	skinCB.get()->CopyData(0, *boneStateNow);
}
void RenderItem::Render(ComPtr<ID3D12GraphicsCommandList> mCommandList)
{
	if (IsBaseObj)
	{
		mCommandList->IASetVertexBuffers(0, 4, Geo->mVBV);
		mCommandList->SetGraphicsRootConstantBufferView(1,0);

	}
	else
	{
		mCommandList->IASetVertexBuffers(0, 5, Geo->mVBV);
		mCommandList->SetGraphicsRootConstantBufferView(1, GetSkinCB()->GetGPUVirtualAddress());

	}
	mCommandList->IASetIndexBuffer(&Geo->mIBV);
	mCommandList->IASetPrimitiveTopology(PrimitiveType);
	mCommandList->SetGraphicsRootConstantBufferView(0, ObjectCB.get()->Resource()->GetGPUVirtualAddress());
	
	for (auto m : Geo->DrawArgs)
	{
		auto t= DescriptorManager::GetInstance().GetSRVDescriptorHandle_GPU(MaterialMgr::GetInstance().GetMaterial(m.second.MaterialIndex).DiffuseSrvHeapIndex);
		auto d = DescriptorManager::GetInstance().GetSRVDescriptorHandle_GPU(MaterialMgr::GetInstance().GetMaterial(m.second.MaterialIndex).NormalSrvHeapIndex);
		mCommandList->SetGraphicsRootConstantBufferView(3, MaterialMgr::GetInstance().GetUploadData(m.second.MaterialIndex)->GetGPUVirtualAddress());
		mCommandList->SetGraphicsRootDescriptorTable(7, DescriptorManager::GetInstance().GetSRVDescriptorHandle_GPU(MaterialMgr::GetInstance().GetMaterial(m.second.MaterialIndex).DiffuseSrvHeapIndex));
		mCommandList->SetGraphicsRootDescriptorTable(8, DescriptorManager::GetInstance().GetSRVDescriptorHandle_GPU(MaterialMgr::GetInstance().GetMaterial(m.second.MaterialIndex).NormalSrvHeapIndex));
		mCommandList->DrawIndexedInstanced(m.second.IndexCount, 1, m.second.StartIndexLocation, m.second.BaseVertexLocation, 0);
	}
}

void RenderItem::UpdateObjConst(ObjectConstants data)
{
	ObjectCB.get()->CopyData(0, data);
}

void RenderItem::UpdateCB()
{
	if (NumFramesDirty>0)
	{
		ObjectConstants data;
		XMFloat4x4Transpose(&data.World, &m_ModelTranformation);
		XMFloat4x4Transpose(&data.TexTransform, &TexTransform);
		data.MaterialIndex = 0;
		data.ObjPad0 = 0;
		data.ObjPad1 = 0;
		data.ObjPad2 = 0;
		ObjectCB.get()->CopyData(0, data);
		NumFramesDirty = 0;
	}
	

}

void RenderItem::StartFunc()
{
	for (size_t i = 0; i < startFuncs.size(); i++)
	{
		startFuncs[i]();
	}
}

void RenderItem::UpdateFunc()
{
	for (size_t i = 0; i < updateFuncs.size(); i++)
	{
		updateFuncs[i]();
	}
}

void RenderItem::DestoryFunc()
{
	for (size_t i = 0; i < destoryFuncs.size(); i++)
	{
		destoryFuncs[i]();
	}
}

void RenderItem::Move(float x, float y, float z, float speed)
{
	XMVECTOR direct = XMVectorSet(x*speed, y*speed, z*speed, 0);
	XMMATRIX trans = XMLoadFloat4x4(&m_ModelTranformation);
	XMMATRIX dir = XMMatrixTranslationFromVector(direct);
	trans *= dir;
	XMStoreFloat4x4(&m_ModelTranformation, trans);
}

void RenderItem::AddScript(string path)
{
	scriptName.push_back(path);
}

void RenderItem::PrintName()
{
	LogSystem::GetInstance().log(LogSystem::LogLevel::debug,m_strFileName);
}

vector<string> RenderItem::GetScriptName()
{
	return scriptName;
}


ID3D12Resource* RenderItem::GetObjCB()
{
	return ObjectCB.get()->Resource();
}

ID3D12Resource* RenderItem::GetSkinCB()
{
	if (IsBaseObj)
		return 0;
	return skinCB.get()->Resource();
}


