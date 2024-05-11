#include"PsoResource.h"

psoResource::psoResource()
{
	ifstream ou(PSO_RESOURCE_PATH);
	cereal::BinaryInputArchive ij(ou);
	ij(resource);
}

psoResource::~psoResource()
{
	ofstream os(PSO_RESOURCE_PATH);
	cereal::BinaryOutputArchive Achive(os);
	Achive(resource);
}

PsoResAll psoResource::GetPsoResource()
{
	return resource;
}
