#pragma once
#include"renderPipeline.h"
#include"MaterialManager.h"
#define ASSIMP_LOAD_FLAGS_DEFAULT (aiProcess_Triangulate\
 | aiProcess_GenSmoothNormals\
 | aiProcess_GenBoundingBoxes\
 | aiProcess_JoinIdenticalVertices\
 | aiProcess_FlipUVs\
 | aiProcess_ConvertToLeftHanded\
 | aiProcess_LimitBoneWeights)	
struct Bonedata
{
	XMMATRIX offsetMatrix;
	XMMATRIX m_mxFinalTransformation;
};
struct VectorKey
{
	double mTime;
	XMFLOAT3 mValue;
};
struct QuaternionKey
{
	double mTime;
	XMFLOAT4 mValue;
};
struct NodeAnim
{
	UINT parentIndex;
	string Bonename;
	vector<VectorKey> mPositionKeys;
	vector<QuaternionKey> mRotationKeys;
	vector<VectorKey> mScalingKeys;
};
class AnimData
{
public:
	double mDuration;
	double mTicksPerSecond;
	map<string, NodeAnim> BoneAnimChannel;
private:

};
struct AllBones
{
	XMMATRIX mxBones[256];    //骨骼动画“调色板”
};
struct myVertexBone
{
public:
	UINT32  m_nBonesIDs[4];
	FLOAT	m_fWeights[4];
public:
	VOID AddBoneData(UINT nBoneID, FLOAT fWeight)
	{
		for (UINT32 i = 0; i < 4; i++)
		{
			if (m_fWeights[i] == 0.0)
			{
				m_nBonesIDs[i] = nBoneID;
				m_fWeights[i] = fWeight;
				break;
			}
		}
	}
};
struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT     MaterialIndex;
	UINT     ObjPad0;
	UINT     ObjPad1;
	UINT     ObjPad2;
};

class RenderItem
{
public:
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;
	XMFLOAT4X4 m_ModelTranformation = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;
	string		m_strFileName;
	bool IsBaseObj = false;
	myPso* pipeline=nullptr;
	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	bool initAsSkinModel(string pszFileName, UINT nFlags = ASSIMP_LOAD_FLAGS_DEFAULT);
	bool initAsBox(float width, float height, float depth, UINT numSubdivisions);
	bool initAsGrid(float width, float depth, UINT m, UINT n);
	bool initAsSphere(float radius, UINT sliceCount, UINT stackCount);
	bool initAsCylinder(float bottomRadius, float topRadius, float height, UINT sliceCount, UINT stackCount);
	bool initAsQuad(float x, float y, float w, float h, float depth);
	bool initScript();
	void SetMaterial(string materialName);
	void SetPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY type);
	void SetScale(XMFLOAT3 Scale);
	HRESULT UploadModel(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12CommandAllocator* cmdAlloc, ID3D12GraphicsCommandList* cmdList);
	void BoneAnimePlay(int animIndex, double frameDelteTime);
	void Render(ComPtr<ID3D12GraphicsCommandList> mCommandList);
	void UpdateObjConst(ObjectConstants data);
	void UpdateCB();
	void StartFunc();
	void UpdateFunc();
	void DestoryFunc();
	void Move(float x, float y, float z, float speed);
	void AddScript(string path);
	void PrintName();
	vector<string> GetScriptName();
	ID3D12Resource* GetObjCB();
	ID3D12Resource* GetSkinCB();
private:
	UINT  nodeQueueReadNum[256][3] = { 0 };
	AllBones* boneStateNow;
	map<string, UINT> m_mapNodeName2BoneIndex;
	map<string, UINT>	m_mapName2Bone;			//名称->骨骼的索引
	UINT				m_nCurrentAnimIndex;

	const aiScene* m_paiModel;

	MeshGeometry* Geo;
	vector<XMFLOAT4> Vertices;
	vector<XMFLOAT4> Normals;
	vector<XMFLOAT4> Tangents;
	vector<XMFLOAT4> Bitangents;
	vector<XMFLOAT2> TextureCoords;
	vector<UINT> FaceIndex;
	vector<myVertexBone> vectorBoneWeights;
	vector<Bonedata*> BoneData;
	map<UINT, UINT> local2worldMaterialIndex;

	map<string, AnimData> boneAnimationData;
	void CalculateNodeAnim(aiNode* root, XMMATRIX parentTransform, AnimData* anime, int* i);
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
	std::unique_ptr<UploadBuffer<AllBones>> skinCB = nullptr;
	vector<void(*)()> startFuncs;
	vector<void(*)()> updateFuncs;
	vector<void(*)()> destoryFuncs;
	vector<string> scriptName;
};