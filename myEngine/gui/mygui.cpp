#include"mygui.h"
int selected = -1;
int minValue = -100; // 滑动条的最小值
int maxValue = 100; // 滑动条的最大值
int currentItem = 0; //script index
XMFLOAT3 rotateEngul;
XMFLOAT3 scale;
struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                  FenceValue;
};
void init()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(SkinnedMeshApp::GetInstance().GetMainWnd());
    ImGui_ImplDX12_Init(SkinnedMeshApp::GetInstance().GetDevice().Get(), 3,
        DXGI_FORMAT_R8G8B8A8_UNORM, SkinnedMeshApp::GetInstance().GetSrvHeap().Get(),
        SkinnedMeshApp::GetInstance().GetSrvHeap().Get()->GetCPUDescriptorHandleForHeapStart(),
        SkinnedMeshApp::GetInstance().GetSrvHeap().Get()->GetGPUDescriptorHandleForHeapStart());
}
void close()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
XMFLOAT3 XMQuaternionToEuler(FXMVECTOR q)
{
    XMMATRIX res = XMMatrixRotationQuaternion(q);
    XMFLOAT4X4 R;
    XMStoreFloat4x4(&R, res);
    float x, y, z;
    y = atan2f(R._21, R._22); // pitch
    x = atan2f(R._31, sqrt(R._11 * R._11 + R._21 * R._21)); // roll
    z = atan2f(R._12, R._11); // yaw
    XMFLOAT3 o(x, y, z);
    return o;
}
void ProperWindow(int num,RenderItem* item)
{
    if (ImGui::Begin("proper window"))
    {
        float winWidth = ImGui::GetWindowWidth();
        ImGui::PushItemWidth(winWidth / 3);
        XMMATRIX trans = XMLoadFloat4x4(&item->m_ModelTranformation);
        auto rotate=XMQuaternionRotationMatrix(trans);
        rotateEngul = XMQuaternionToEuler(rotate);
        ImGui::Text("ObjName--");
        ImGui::SameLine(winWidth / 3);
        ImGui::Text(item->m_strFileName.c_str());


        ImGui::Text("Trans_X");
        ImGui::SameLine(winWidth/3);
        ImGui::Text("Trans_Y");
        ImGui::SameLine(winWidth*2 / 3);
        ImGui::Text("Trans_Z");
        if (ImGui::SliderFloat("##Trans_X", &item->m_ModelTranformation._41, minValue, maxValue))
        {

        }
        ImGui::SameLine();
        if (ImGui::SliderFloat("##Trans_Y", &item->m_ModelTranformation._42, minValue, maxValue))
        {

        }
        ImGui::SameLine();
        if (ImGui::SliderFloat("##Trans_Z", &item->m_ModelTranformation._43, minValue, maxValue))
        {

        }   
        ImGui::Text("scale_X");
        ImGui::SameLine(winWidth / 3);
        ImGui::Text("scale_Y");
        ImGui::SameLine(winWidth * 2 / 3);
        ImGui::Text("scale_Z");
        if (ImGui::SliderFloat("##scale_X", &item->m_ModelTranformation._11, minValue, maxValue))
        {
        }
        ImGui::SameLine();
        if (ImGui::SliderFloat("##scale_Y", &item->m_ModelTranformation._22, minValue, maxValue))
        {

        }
        ImGui::SameLine();
        if (ImGui::SliderFloat("##scale_Z", &item->m_ModelTranformation._33, minValue, maxValue))
        {

        }
        string scriptName = "without script";
        if (item->GetScriptName().size()>0)
        {
            scriptName = item->GetScriptName()[currentItem];
        }
        if (ImGui::BeginCombo("script",scriptName.c_str() )) // 第一个参数是下拉列表的标识符，第二个参数是当前选中项的名称
        {
            bool item_selected = false; // 标记是否选中了某个项
            for (int i = 0; i < item->GetScriptName().size(); i++)
            {
                if (ImGui::Selectable(item->GetScriptName()[i].c_str(), currentItem == i))
                {
                    currentItem = i; // 更新当前选中项
                }
            }

            ImGui::EndCombo(); // 结束下拉列表
        }
        ImGui::PopItemWidth();
        ImGui::End();
    }
}
void draw()
{
    try
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Game objects"))                       // Create a window called "Hello, world!" and append into it.
        {
            auto renderItems = SkinnedMeshApp::GetInstance().GetRenderItem();
            if (renderItems.size() > 0)
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
                if (ImGui::BeginListBox("##object List", ImVec2(-FLT_MIN, -FLT_MIN))) {

                    for (int i = 0; i < renderItems.size(); i++) {
                        if (ImGui::Selectable(("OBJ"+to_string(i) + "-" + renderItems[i]->m_strFileName).c_str(), selected == i)) {
                            selected = i;
                            LogSystem::GetInstance().log(LogSystem::LogLevel::info, "open properWindow--"+to_string(i));
                        }
                    }
                    ImGui::EndListBox();
                }
                ImGui::PopStyleColor();
            }
            if (selected>=0)
            {
                ProperWindow(selected,renderItems[selected]);
            }

            ImGui::End();
        }
        ImGui::Render();
        SkinnedMeshApp::GetInstance().GetCommandList()->SetDescriptorHeaps(1, SkinnedMeshApp::GetInstance().GetSrvHeap().GetAddressOf());
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), SkinnedMeshApp::GetInstance().GetCommandList().Get());
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA(e.what());
    }
   
}

mygui::mygui()
{
}

void mygui::initGui()
{
    SkinnedMeshApp::GetInstance().AddInitFunc(init);
    SkinnedMeshApp::GetInstance().AddDestoryFunc(close);
    SkinnedMeshApp::GetInstance().AddDrawFunc(draw);
    SkinnedMeshApp::GetInstance().AddMsgProcFunc(ImGui_ImplWin32_WndProcHandler);
    LogSystem::GetInstance().log(LogSystem::LogLevel::info, "Init myGui");
}

