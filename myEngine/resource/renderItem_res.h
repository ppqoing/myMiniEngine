#pragma once
#include"../baseUtil.h"
#define Render_Obj_resource_path "baseFile/RenderItem.resource"
struct render_res
{
	string name;
	float m_ModelTranformation[4][4] = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
	float TexTransform[4][4] = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 };
	string material;
	void setTransform(XMFLOAT4X4 t)
	{
		m_ModelTranformation[0][0] = t._11;
		m_ModelTranformation[0][1] = t._12;
		m_ModelTranformation[0][2] = t._13;
		m_ModelTranformation[0][3] = t._14;
		m_ModelTranformation[1][0] = t._21;
		m_ModelTranformation[1][1] = t._22;
		m_ModelTranformation[1][2] = t._23;
		m_ModelTranformation[1][3] = t._24;
		m_ModelTranformation[2][0] = t._31;
		m_ModelTranformation[2][1] = t._32;
		m_ModelTranformation[2][2] = t._33;
		m_ModelTranformation[2][3] = t._34;
		m_ModelTranformation[3][0] = t._41;
		m_ModelTranformation[3][1] = t._42;
		m_ModelTranformation[3][2] = t._43;
		m_ModelTranformation[3][3] = t._44;

	}
	void setTex(XMFLOAT4X4 t)
	{
		TexTransform[0][0] = t._11;
		TexTransform[0][1] = t._12;
		TexTransform[0][2] = t._13;
		TexTransform[0][3] = t._14;
		TexTransform[1][0] = t._21;
		TexTransform[1][1] = t._22;
		TexTransform[1][2] = t._23;
		TexTransform[1][3] = t._24;
		TexTransform[2][0] = t._31;
		TexTransform[2][1] = t._32;
		TexTransform[2][2] = t._33;
		TexTransform[2][3] = t._34;
		TexTransform[3][0] = t._41;
		TexTransform[3][1] = t._42;
		TexTransform[3][2] = t._43;
		TexTransform[3][3] = t._44;
	}
};
struct skinModel_res :render_res
{
	string filePath;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(name, filePath, m_ModelTranformation, TexTransform);
	}
};
struct BoxRes :render_res
{
	float width;
	float height;
	float depth;
	UINT numSubdivisions;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(name, m_ModelTranformation, TexTransform, width, height, depth, numSubdivisions);
	}
};
struct GridRes :render_res
{
	float width;
	float depth;
	UINT m;
	UINT n;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(name, m_ModelTranformation, TexTransform, width, depth, m, n);
	}
};
struct SphereRes :render_res
{
	float radius;
	UINT sliceCount;
	UINT stackCount;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(name, m_ModelTranformation, TexTransform, radius, sliceCount, stackCount);
	}
};
struct CylinderRes :render_res
{
	float bottomRadius;
	float topRadius;
	float height;
	UINT sliceCount;
	UINT stackCount;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(name, m_ModelTranformation, TexTransform, bottomRadius, topRadius, height, sliceCount, stackCount);
	}
};
struct QuadRes :render_res
{
	float x, y, w, h;
	float depth;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(name, m_ModelTranformation, TexTransform, x, y, w, h, depth);
	}
};
struct renderItem_res
{
	vector<skinModel_res> skinModels;
	vector<BoxRes> boxs;
	vector<GridRes> grids;
	vector<SphereRes> spheres;
	vector<CylinderRes> cylinders;
	vector<QuadRes> quads;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(skinModels, boxs, grids, spheres, cylinders, quads);
	}
};

class renderItemResource:public Sigleton<renderItemResource>
{
	friend class Sigleton<renderItemResource>;
public:
	renderItemResource();
	~renderItemResource();
	void AddskinModel_res(skinModel_res skin);
	void AddboxRes(BoxRes box);
	void AddGrid(GridRes grid);
	void AddSphere(SphereRes sphere);
	void AddCylinder(CylinderRes cylin);
	void AddQuad(QuadRes quad);
	void Save2File();
	void clean();
	renderItem_res GetRenderItem();
private:
	renderItem_res res;
};