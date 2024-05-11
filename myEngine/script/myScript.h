#pragma once
#include"mono_my/jit/jit.h"
#include "mono_my/metadata/assembly.h"
#include "mono_my/metadata/class.h"
#include "mono_my/metadata/debug-helpers.h"
#include "mono_my/metadata/mono-config.h"
#include"../SkinnedMeshApp.h"
#define SCRIPT_DLL_PATH "csharp/myMono.dll"
class instanceObj
{
public:
	MonoObject* instance;
	MonoMethod* awake;
	MonoMethod* start;
	MonoMethod* update;
	MonoMethod* destory;
	MonoClassField* indexField;
};
void Awake();
void Start();
void Update();
void Destory();
void LogCsharp(MonoString* logMessage);
float deltaTime();
void PrintName(int index);
extern "C" void Move_item(int index, float x, float y, float z, float speed);

class myScript:public Sigleton<myScript>
{
	friend class Sigleton<myScript>;
public:
	myScript();
	~myScript();
	void AddFuncToScript(string funcName, const void* method);
	void InstanceScriptObj(RenderItem* item,string scriptName);
	void initCsharpBridge();

	void init();
private:
	void initCsharpFunc();

};