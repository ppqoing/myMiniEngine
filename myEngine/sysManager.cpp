#include "sysManager.h"

void CreateConsoleWindow()
{
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CON", "r", stdin);
    freopen_s(&stream, "CON", "w", stdout);
}
sysManager::sysManager()
{
    CreateConsoleWindow();
	LogSystem::GetInstance().log(LogSystem::LogLevel::info, "Init SysManager");
}

void sysManager::init(HINSTANCE hInstance)
{
    try
    {
        
        mygui::GetInstance().initGui();        
        if (!SkinnedMeshApp::GetInstance().Initialize(hInstance))
            LogSystem::GetInstance().log(LogSystem::LogLevel::error, "Init renderSystem error");
        myScript::GetInstance().init();
        SkinnedMeshApp::GetInstance().InitScript();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        LogSystem::GetInstance().log(LogSystem::LogLevel::error, "Init error");
    }
}

void sysManager::run()
{
    SkinnedMeshApp::GetInstance().Run();
}

sysManager::~sysManager()
{
}
