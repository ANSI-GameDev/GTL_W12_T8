#pragma once
#include "WindowsCursor.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Engine/EditorEngine.h"
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/UnrealEd.h"

//자신의 꿈을 펼쳐볼 수 있는 서브 엔진입니다.
class USubEngine : public UEngine
{
    DECLARE_CLASS(USubEngine, UEngine)
public:
    USubEngine();
    ~USubEngine();

    virtual void Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow, UnrealEd* InUnrealEd);
    virtual void Tick(float DeltaTime) override;
    virtual void PhysicsTick(float DeltaTime) override;
    virtual void Input(float DeltaTime);
    virtual void Render();
    virtual void Release();
    void RequestShowWindow(bool bShow);

public:
    HWND* Wnd;
    FGraphicsDevice* Graphics;
    FDXDBufferManager* BufferManager;
    class FSubRenderer* SubRenderer;
    UnrealEd* UnrealEditor;
    class FImGuiSubWindow* SubUI;

    AEditorPlayer* EditorPlayer;

    POINT LastMousePos;
    bool bRBClicked = false;

    std::shared_ptr<FEditorViewportClient> ViewportClient;
    bool bIsShowSubWindow;
    bool bIsShowing = false;

    AActor* SelectedActor = nullptr;
    USceneComponent* SelectedComponent = nullptr;
};
