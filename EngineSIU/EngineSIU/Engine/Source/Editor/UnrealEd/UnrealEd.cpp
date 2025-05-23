#include "UnrealEd.h"
#include "EditorPanel.h"

#include "PropertyEditor/ControlEditorPanel.h"
#include "PropertyEditor/OutlinerEditorPanel.h"
#include "PropertyEditor/PropertyEditorPanel.h"
#include "PropertyEditor/SkeletalMeshViewerPanel.h"
#include "PropertyEditor/SubEditor/ParticleViewerPanel.h"
#include "PropertyEditor/SubEditor/PhysicsViewerPanel.h"
#include "World/World.h"
void UnrealEd::Initialize()
{
    auto ControlPanel = std::make_shared<ControlEditorPanel>();
    Panels["ControlPanel"] = ControlPanel;

    auto OutlinerPanel = std::make_shared<OutlinerEditorPanel>();
    Panels["OutlinerPanel"] = OutlinerPanel;

    auto PropertyPanel = std::make_shared<PropertyEditorPanel>();
    Panels["PropertyPanel"] = PropertyPanel;

    // TODO : SkeletalViewe 전용 UI 분리
    auto BoneHierarchyPanel = std::make_shared<SkeletalMeshViewerPanel>();
    Panels["BoneHierarchyPaenl"] = BoneHierarchyPanel;

    auto ParticlePanel = std::make_shared<ParticleViewerPanel>();
    ParticlePanel->WindowType = WT_ParticleSubWindow;
    ParticleSubPanels["ParticleViewerPanel"] = ParticlePanel;

    auto PhysicsPanel = std::make_shared<PhysicsViewerPanel>();
    PhysicsPanel->WindowType = WT_PhysicsSubWindow;
    PhysicsSubPanels["PhysicsViewerPanel"] = PhysicsPanel;
}

void UnrealEd::Render(EWindowType WindowType) const
{
    if (WindowType == EWindowType::WT_Main)
    {
        EWorldTypeBitFlag currentMask;
        switch (GEngine->ActiveWorld->WorldType)
        {
        case EWorldType::Game:
            currentMask = EWorldTypeBitFlag::Game;
            break;
        case EWorldType::Editor:
            currentMask = EWorldTypeBitFlag::Editor;
            break;
        case EWorldType::PIE:
            currentMask = EWorldTypeBitFlag::PIE;
            break;
        case EWorldType::EditorPreview:
            currentMask = EWorldTypeBitFlag::EditorPreview;
            break;
        case EWorldType::GamePreview:
            currentMask = EWorldTypeBitFlag::GamePreview;
            break;
        case EWorldType::GameRPC:
            currentMask = EWorldTypeBitFlag::GameRPC;
            break;
        case EWorldType::SkeletalViewer:
            currentMask = EWorldTypeBitFlag::SkeletalViewer;
            break;
        case EWorldType::Inactive:
            currentMask = EWorldTypeBitFlag::Inactive;
            break;
        default:
            currentMask = EWorldTypeBitFlag::None;
            break;

        }
        for (const auto& Panel : Panels)
        {
            if (HasFlag(Panel.Value->GetSupportedWorldTypes(), currentMask))
            {
                Panel.Value->Render();
            }
        }
    }
    else if (WindowType == EWindowType::WT_ParticleSubWindow)
    {
        for (const auto& Panel : ParticleSubPanels)
        {
            if (Panel.Value)
            {
                Panel.Value->Render();
            }
        }
        return;
    }
    else if (WindowType == EWindowType::WT_PhysicsSubWindow)
    {
        for (const auto& Panel : PhysicsSubPanels)
        {
            if (Panel.Value)
            {
                Panel.Value->Render();
            }
        }
        return;
    }
}

void UnrealEd::AddEditorPanel(const FString& PanelId, const std::shared_ptr<UEditorPanel>& EditorPanel)
{
    Panels[PanelId] = EditorPanel;
}

void UnrealEd::OnResize(HWND hWnd, EWindowType WindowType) const
{
    if (WindowType == EWindowType::WT_Main)
    {
        for (auto& Panel : Panels)
        {
            if (Panel.Value)
            {
                Panel.Value->OnResize(hWnd);
            }
        }
    }
    else if (WindowType == EWindowType::WT_ParticleSubWindow)
    {
        for (auto& Panel : ParticleSubPanels)
        {
            if (Panel.Value)
            {
                Panel.Value->OnResize(hWnd);
            }
        }
    }
    else if (WindowType == EWindowType::WT_PhysicsSubWindow)
    {
        for (auto& Panel : PhysicsSubPanels)
        {
            if (Panel.Value)
            {
                Panel.Value->OnResize(hWnd);
            }
        }
    }
}

std::shared_ptr<UEditorPanel> UnrealEd::GetEditorPanel(const FString& PanelId)
{
    return Panels[PanelId];
}

std::shared_ptr<UEditorPanel> UnrealEd::GetParticleSubPanel(const FString& PanelId)
{
    return ParticleSubPanels[PanelId];
}
std::shared_ptr<UEditorPanel> UnrealEd::GetPhysicsSubPanel(const FString& PanelId)
{
    return PhysicsSubPanels[PanelId];
}
