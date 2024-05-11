#pragma once
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#include"../SkinnedMeshApp.h"
using namespace Microsoft::WRL;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class mygui:public Sigleton<mygui>
{
	friend class Sigleton<mygui>;
public:
	mygui();
	void initGui();
private:
	
};