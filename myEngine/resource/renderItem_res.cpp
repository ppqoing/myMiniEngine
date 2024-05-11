#include"renderItem_res.h"
renderItemResource::renderItemResource()
{
	ifstream f(Render_Obj_resource_path);
	cereal::BinaryInputArchive ba(f);
	ba(res);
}

renderItemResource::~renderItemResource()
{

}

void renderItemResource::AddskinModel_res(skinModel_res skin)
{
	res.skinModels.push_back(skin);
}

void renderItemResource::AddboxRes(BoxRes box)
{
	res.boxs.push_back(box);
}

void renderItemResource::AddGrid(GridRes grid)
{
	res.grids.push_back(grid);
}

void renderItemResource::AddSphere(SphereRes sphere)
{
	res.spheres.push_back(sphere);
}

void renderItemResource::AddCylinder(CylinderRes cylin)
{
	res.cylinders.push_back(cylin);
}

void renderItemResource::AddQuad(QuadRes quad)
{
	res.quads.push_back(quad);
}

void renderItemResource::Save2File()
{
	ofstream of(Render_Obj_resource_path);
	cereal::BinaryOutputArchive bo(of);
	bo(res);
}

void renderItemResource::clean()
{
	res.boxs.clear();
	res.skinModels.clear();
	res.grids.clear();
	res.spheres.clear();
	res.cylinders.clear();
	res.quads.clear();
}

renderItem_res renderItemResource::GetRenderItem()
{
	return res;
}
