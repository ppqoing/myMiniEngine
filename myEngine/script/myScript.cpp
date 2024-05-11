#include"myScript.h"
int index_script = 0;
map<int, RenderItem*> renderItemPool;
map<string, MonoClass*> monoClassPool;
map<string, MonoClassField*> monoFieldPool;
vector<instanceObj> monoInstancePool;
map<string, MonoMethod*> awakeMethod;
map<string, MonoMethod*> startMethod;
map<string, MonoMethod*> updateMethod;
map<string, MonoMethod*> destoryMethod;
MonoAssembly* assembly;
MonoImage* image;
MonoDomain* domain;
myScript::myScript()
{
}

myScript::~myScript()
{
	mono_jit_cleanup(domain);
}

void myScript::AddFuncToScript(string funcName, const void* method)
{
	string funcIndex = "myMono.Component::" + funcName;
	mono_add_internal_call(funcIndex.c_str(), method);
	LogSystem::GetInstance().log(LogSystem::LogLevel::debug, funcIndex);
}
void myScript::InstanceScriptObj(RenderItem* item, string scriptName)
{
	renderItemPool[index_script] = item;
	MonoClass* main_class;
	MonoClassField* monoField;
	instanceObj temp;
	if (monoClassPool.find(scriptName)==monoClassPool.end())
	{
		main_class = mono_class_from_name(image, "myMono", scriptName.c_str());
		monoField = mono_class_get_field_from_name(main_class, "index");
		monoClassPool["myMono" + scriptName] = main_class;
		monoFieldPool["myMono" + scriptName + "index"] = monoField;
		MonoMethodDesc* entry_point_method_desc_awake = mono_method_desc_new(("myMono."+scriptName+"::Awake()").c_str(), true);
		MonoMethodDesc* entry_point_method_desc_start = mono_method_desc_new(("myMono." + scriptName + "::Start()").c_str(), true);
		MonoMethodDesc* entry_point_method_desc_update = mono_method_desc_new(("myMono." + scriptName + "::Update()").c_str(), true);
		MonoMethodDesc* entry_point_method_desc_destory = mono_method_desc_new(("myMono." + scriptName + "::Destory()").c_str(), true);
		MonoMethod* entry_point_method_start = mono_method_desc_search_in_class(entry_point_method_desc_start, main_class);
		MonoMethod* entry_point_method_awake = mono_method_desc_search_in_class(entry_point_method_desc_awake, main_class);
		MonoMethod* entry_point_method_update = mono_method_desc_search_in_class(entry_point_method_desc_update, main_class);
		MonoMethod* entry_point_method_destory = mono_method_desc_search_in_class(entry_point_method_desc_destory, main_class);
		mono_method_desc_free(entry_point_method_desc_awake);
		mono_method_desc_free(entry_point_method_desc_start);
		mono_method_desc_free(entry_point_method_desc_update);
		mono_method_desc_free(entry_point_method_desc_destory);
		awakeMethod["myMono." + scriptName ] = entry_point_method_awake;
		startMethod["myMono." + scriptName ] = entry_point_method_start;
		updateMethod["myMono." + scriptName ] = entry_point_method_update;
		destoryMethod["myMono." + scriptName ] = entry_point_method_destory;
	}
	else
	{
		main_class = monoClassPool["myMono" + scriptName];
		monoField = monoFieldPool["myMono" + scriptName + "index"];
	}
	MonoObject* instance_t = mono_object_new(domain, main_class);
	renderItemPool[index_script] = item;
	mono_field_set_value(instance_t, monoField, &index_script);
	temp.instance = instance_t;
	temp.indexField = monoField;
	temp.awake = awakeMethod["myMono." + scriptName];
	temp.start = startMethod["myMono." + scriptName];
	temp.update = updateMethod["myMono." + scriptName];
	temp.destory = destoryMethod["myMono." + scriptName];
	monoInstancePool.push_back(temp);
	if (temp.awake)
		mono_runtime_invoke(temp.awake, temp.instance, NULL, NULL);
	index_script++;
}
void myScript::initCsharpBridge()
{
	auto items = SkinnedMeshApp::GetInstance().GetRenderItem();
	for (size_t i = 0; i < items.size(); i++)
	{
		auto s_name = items[i]->GetScriptName();
		for (size_t j = 0; j < s_name.size(); j++)
		{
			InstanceScriptObj(items[i], s_name[j]);
		}
	}

}
void Awake()
{
	for (auto i:monoInstancePool)
	{
		if (i.awake)
		mono_runtime_invoke(i.awake, i.instance, NULL, NULL);
	}
}
void Start()
{
	for (auto i : monoInstancePool)
	{
		if (i.start)
		mono_runtime_invoke(i.start, i.instance, NULL, NULL);
	}
}
void Update()
{
	for (auto i : monoInstancePool)
	{
		if (i.update)
		{
			mono_runtime_invoke(i.update, i.instance, NULL, NULL);
		}
	}
}
void Destory()
{
	for (auto i : monoInstancePool)
	{
		if (i.destory)
			mono_runtime_invoke(i.destory, i.instance, NULL, NULL);
	}
}
void myScript::init()
{
	domain = mono_jit_init("teee");
	assembly = mono_domain_assembly_open(domain, SCRIPT_DLL_PATH);
	image = mono_assembly_get_image(assembly);
	initCsharpFunc();
	initCsharpBridge();
	SkinnedMeshApp::GetInstance().AddInitFunc(Start);
	SkinnedMeshApp::GetInstance().AddUpdateFunc(Update);
	SkinnedMeshApp::GetInstance().AddDestoryFunc(Destory);
}
void myScript::initCsharpFunc()
{
	AddFuncToScript("Log", LogCsharp);
	AddFuncToScript("deltaTime", deltaTime);
	AddFuncToScript("PrintName", PrintName);
	AddFuncToScript("Move",Move_item);
}
void LogCsharp(MonoString * logMessage)
{
	std::string LogMess = mono_string_to_utf8(logMessage);
	LogSystem::GetInstance().log(LogSystem::LogLevel::debug, LogMess);
}

float deltaTime()
{
	return SkinnedMeshApp::GetInstance().DeltaTime();
}

void PrintName(int index)
{
	renderItemPool[index]->PrintName();
}
void Move_item(int index,float x,float y,float z, float speed)
{
	LogSystem::GetInstance().log(LogSystem::LogLevel::debug, to_string(x) + to_string(y) + to_string(speed));
	renderItemPool[index]->Move(x,y,z, speed);
}