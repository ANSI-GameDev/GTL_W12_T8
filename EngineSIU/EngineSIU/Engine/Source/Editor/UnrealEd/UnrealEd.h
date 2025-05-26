#pragma once
#include "Container/Map.h"
#include "Container/String.h"
#include "EditorPanel.h"

class UEditorPanel;

class UnrealEd
{
public:
    UnrealEd() = default;
    ~UnrealEd() = default;
    void Initialize();

    void Render(EWindowType WindowType = EWindowType::WT_Main) const;
     void OnResize(HWND hWnd, EWindowType WindowType = EWindowType::WT_Main) const;
    
    void AddEditorPanel(const FString& PanelId, const std::shared_ptr<UEditorPanel>& EditorPanel);
    std::shared_ptr<UEditorPanel> GetEditorPanel(const FString& PanelId);
    std::shared_ptr<UEditorPanel> GetParticleSubPanel(const FString& PanelId);
    std::shared_ptr<UEditorPanel> GetPhysicsSubPanel(const FString& PanelId);

private:
    TMap<FString, std::shared_ptr<UEditorPanel>> Panels;
    //TODO
    //나중에 SubPanel 통합 관리하도록 리팩토링    
    TMap<FString, std::shared_ptr<UEditorPanel>> ParticleSubPanels;
    TMap<FString, std::shared_ptr<UEditorPanel>> PhysicsSubPanels;

};
