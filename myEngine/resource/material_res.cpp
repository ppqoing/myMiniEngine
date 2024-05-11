#include"material_res.h"
MaterialResource::MaterialResource()
{
	ifstream f(Material_File_Path);
	cereal::BinaryInputArchive ba(f);
	ba(material_All);
	f.close();
}

MaterialResource::~MaterialResource()
{
	ofstream of(Material_File_Path);
	cereal::BinaryOutputArchive bo(of);
	bo(material_All);
	of.close();
}

void MaterialResource::AddMaterial(material_res resource)
{
	material_All.m_all.push_back(resource);
}

void MaterialResource::SaveToFile()
{
	ofstream of(Material_File_Path);
	cereal::BinaryOutputArchive bo(of);
	bo(material_All);
}

material_vector MaterialResource::GetMaterial()
{
	return material_All;
}
