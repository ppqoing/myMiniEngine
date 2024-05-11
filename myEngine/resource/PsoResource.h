#pragma once
#include"../baseUtil.h"
#define PSO_RESOURCE_PATH "baseFile/Pso.resource"
struct PsoRes
{
	string name;
	string VsName;
	string PsName;
	psoStyle style;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(name, VsName, PsName,style);
	}
};
struct PsoResAll
{
	vector<PsoRes> psoRes;
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(psoRes);
	}
};
class psoResource:public Sigleton<psoResource>
{
	friend class Sigleton<psoResource>;
public:
	psoResource();
	~psoResource();
	PsoResAll GetPsoResource();
private:
	PsoResAll resource;
};