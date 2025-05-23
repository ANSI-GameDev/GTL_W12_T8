#include "PhysicsViewerPanel.h"

#include "UnrealClient.h"

void PhysicsViewerPanel::Render()
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, Height));

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("PhysicsViewer", nullptr, windowFlags))
    {
        RenderViewportPanel();
        ImGui::Separator();
        RenderPhysicsSettings();
        ImGui::Separator();
        RenderInfoPanel();
        ImGui::End();
    }
}


void PhysicsViewerPanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    if (hWnd && GetClientRect(hWnd, &clientRect))
    {
        Width = static_cast<float>(clientRect.right - clientRect.left);
        Height = static_cast<float>(clientRect.bottom - clientRect.top);
    }
}
void PhysicsViewerPanel::SetViewportClient(std::shared_ptr<FEditorViewportClient> InViewportClient)
{
    ViewportClient = InViewportClient;
}

void PhysicsViewerPanel::RenderViewportPanel()
{
    if (!ViewportClient) return;

    FViewport* Viewport = ViewportClient->GetViewport();
    if (!Viewport) return;

    FViewportResource* Resource = Viewport->GetViewportResource();
    if (!Resource) return;

    FRenderTargetRHI* RenderTarget = Resource->GetRenderTarget(EResourceType::ERT_Scene);
    if (RenderTarget && RenderTarget->SRV)
    {
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)(RenderTarget->SRV), contentSize);
    }
}

void PhysicsViewerPanel::RenderPhysicsSettings()
{
    /*ImGui::Text("Physics Settings:");
    ImGui::Checkbox("Enable Gravity", /* TODO: Hook to simulation setting #1# nullptr);
    ImGui::SliderFloat("Time Step", /* TODO: Hook to sim delta #1# nullptr, 0.001f, 0.033f);*/
}

void PhysicsViewerPanel::RenderInfoPanel()
{
    /*ImGui::Text("Debug Info:");
    ImGui::Text("Body Count: %d", /* TODO: replace with actual data #1# 0);
    ImGui::Text("Broadphase Type: %s", /* TODO: #1# "AABB Grid");*/
}
