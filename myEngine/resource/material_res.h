#pragma once
#include"../cereal.h"
#include"../baseUtil.h"
#define Material_File_Path "baseFile/material.resource"
struct material_res
{
	string m_name = "123";
	wstring DiffuseTexName = L"sdf";
	wstring NormalTexName = L"sdf";
	float DiffuseAlbedo_x = 1.0f;
	float DiffuseAlbedo_y = 2.0f;
	float DiffuseAlbedo_z = 3.0f;
	float DiffuseAlbedo_w = 4.0f;
	float FresnelR0_x = 5.0f;
	float FresnelR0_y = 6.0f;
	float FresnelR0_z = 7.0f;
	float Roughness = 8.0f;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(m_name,
			DiffuseTexName,
			NormalTexName,
			DiffuseAlbedo_x,
			DiffuseAlbedo_y,
			DiffuseAlbedo_z,
			DiffuseAlbedo_w,
			FresnelR0_x,
			FresnelR0_y,
			FresnelR0_z,
			Roughness
		);
	}
};

struct material_vector
{
	vector<material_res> m_all;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(m_all);
	}
};

class MaterialResource:public Sigleton<MaterialResource>
{
	friend class Sigleton<MaterialResource>;
public:
	MaterialResource();
	~MaterialResource();
	void AddMaterial(material_res resource);
	void SaveToFile();
	material_vector GetMaterial();
private:
	material_vector material_All;
};