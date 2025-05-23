// PhysicsSubEngine.cpp
#include "PhysicsSubEngine.h"

#include "ImGuiSubWindow.h"
#include "SubRenderer.h"

UPhysicsSubEngine::UPhysicsSubEngine() {}
UPhysicsSubEngine::~UPhysicsSubEngine() {}

void UPhysicsSubEngine::Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow, UnrealEd* InUnrealEd)
{
    Super::Initialize(hWnd, InGraphics, InBufferManager, InSubWindow, InUnrealEd);

    ViewportClient->CameraReset();
    ViewportClient->ViewFOV = 60.f;
    ViewportClient->SetCameraSpeed(5.0f);

    // 필요한 컴포넌트 로딩이나 초기화 등
}

void UPhysicsSubEngine::Tick(float DeltaTime)
{
    Input(DeltaTime);
    ViewportClient->Tick(DeltaTime);

    // 물리 시뮬레이션 처리 (예: PhysicsWorld->StepSimulation(DeltaTime))

    Render();
}

void UPhysicsSubEngine::Input(float DeltaTime)
{
    if (::GetFocus() != *Wnd) return;
    // 동일한 카메라 조작 구현 (WASDQE + 마우스 우클릭 회전)
}

void UPhysicsSubEngine::Render()
{
    if (Wnd && IsWindowVisible(*Wnd) && Graphics->Device)
    {
        Graphics->Prepare();
        SubRenderer->PrepareRender(ViewportClient);  // Physics 전용 구현 필요
        SubRenderer->Render(ViewportClient);
        SubRenderer->ClearRender();

        SubUI->BeginFrame();
        UnrealEditor->Render(EWindowType::WT_PhysicsSubWindow); // 새 WindowType 추가
        SubUI->EndFrame();

        Graphics->SwapBuffer();
    }
}
void UPhysicsSubEngine::Release()
{
    USubEngine::Release(); // 필요 시 기본 동작 호출
}
