// PhysicsSubEngine.cpp
#include "PhysicsSubEngine.h"

#include "ImGuiSubWindow.h"
#include "SubRenderer.h"
#include "PropertyEditor/SkeletalMeshViewerPanel.h"
#include "PropertyEditor/SubEditor/PhysicsViewerPanel.h"

UPhysicsSubEngine::UPhysicsSubEngine() {}
UPhysicsSubEngine::~UPhysicsSubEngine() {}

void UPhysicsSubEngine::Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow, UnrealEd* InUnrealEd)
{
    Super::Initialize(hWnd, InGraphics, InBufferManager, InSubWindow, InUnrealEd);

    /*ViewportClient->CameraReset();
    ViewportClient->ViewFOV = 60.f;
    ViewportClient->SetCameraSpeed(5.0f);*/
    EditorPlayer = FObjectFactory::ConstructObject<AEditorPlayer>(this);
    EditorPlayer->SetCoordMode(CDM_LOCAL);
    SkeletalMeshComponent = FObjectFactory::ConstructObject<USkeletalMeshComponent>(this);
    FName SkeletalMeshName = "Contents/Asset/Human";
    SkeletalMeshComponent->SetSkeletalMeshAsset(UAssetManager::Get().GetSkeletalMesh(SkeletalMeshName.ToString()));

    //TODO 
    //Comp 세팅 함수 만들고 옮겨야함
    const FBoundingBox& Bounds = SkeletalMeshComponent->GetBoundingBox();
    FVector Center = (Bounds.MaxLocation+Bounds.MinLocation)/2.0f;
    // 카메라 거리 조절용 상수
    const float CameraDistance = 200.0f;

    // 중심을 기준으로 뒤쪽에서 바라보는 위치로 설정
    FVector EyeLocation = Center/* + FVector(-CameraDistance, CameraDistance * 0.5f, CameraDistance * 0.5f)*/;
    EyeLocation += FVector(CameraDistance, 0, 0);
    FVector LookAt = Center;
    FVector Direction = (LookAt - EyeLocation).GetSafeNormal();

    //FRotator ViewRotation = FRotator(Direction);

    ViewportClient->PerspectiveCamera.SetLocation(EyeLocation);
    ViewportClient->PerspectiveCamera.SetRotation(FVector(180,0,-180));


    PhysicsViewerPanel* PhysicsPanel = reinterpret_cast<PhysicsViewerPanel*>(UnrealEditor->GetPhysicsSubPanel("PhysicsViewerPanel").get());
    PhysicsPanel->SetViewportClient(ViewportClient);
    PhysicsPanel->SetSkeletalMeshComponent(SkeletalMeshComponent);

    // 필요한 컴포넌트 로딩이나 초기화 등
    SubRenderer->SetEnabledPass("Skeletal",true);
    ViewportClient->SetViewMode(EViewModeIndex::VMI_Unlit);
}

void UPhysicsSubEngine::Tick(float DeltaTime)
{
    Input(DeltaTime);
    ViewportClient->Tick(DeltaTime);
    FString str = ViewportClient->PerspectiveCamera.GetRotation().ToString();
    UE_LOG(ELogLevel::Warning, "%s", ViewportClient->PerspectiveCamera.GetRotation().ToString());
    // 물리 시뮬레이션 처리 (예: PhysicsWorld->StepSimulation(DeltaTime))

    Render();
}

void UPhysicsSubEngine::Input(float DeltaTime)
{
    if (::GetFocus() != *Wnd)
        return;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        if (!bRBClicked)
        {
            bRBClicked = true;
            GetCursorPos(&LastMousePos);
        }
        POINT CursorPos;
        GetCursorPos(&CursorPos);

        float DeltaX = CursorPos.x - LastMousePos.x;
        float DeltaY = CursorPos.y - LastMousePos.y;
        ViewportClient->CameraRotateYaw(DeltaX * 0.1f);
        ViewportClient->CameraRotatePitch(DeltaY * 0.1f);
        LastMousePos = CursorPos;
    }
    else
    {
        if (bRBClicked)
        {
            bRBClicked = false;
        }
    }


    if (bRBClicked)
    {
        if (GetAsyncKeyState('A') & 0x8000)
        {
            ViewportClient->CameraMoveRight(CameraSpeedMultiplier * DeltaTime);
        }
        if (GetAsyncKeyState('D') & 0x8000)
        {
            ViewportClient->CameraMoveRight(-CameraSpeedMultiplier * DeltaTime);
        }
        if (GetAsyncKeyState('W') & 0x8000)
        {
            ViewportClient->CameraMoveForward(CameraSpeedMultiplier * DeltaTime);
        }
        if (GetAsyncKeyState('S') & 0x8000)
        {
            ViewportClient->CameraMoveForward(-CameraSpeedMultiplier * DeltaTime);
        }
        if (GetAsyncKeyState('E') & 0x8000)
        {
            ViewportClient->CameraMoveUp(CameraSpeedMultiplier * DeltaTime);
        }
        if (GetAsyncKeyState('Q') & 0x8000)
        {
            ViewportClient->CameraMoveUp(-CameraSpeedMultiplier * DeltaTime);
        }
    }
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
        Graphics->DeviceContext->OMSetRenderTargets(
            1,
            &Graphics->BackBufferRTV,
            Graphics->DeviceDSV
        );
        UnrealEditor->Render(EWindowType::WT_PhysicsSubWindow); // 새 WindowType 추가
        SubUI->EndFrame();

        Graphics->SwapBuffer();
    }
}
void UPhysicsSubEngine::Release()
{
    USubEngine::Release(); // 필요 시 기본 동작 호출
}
USkeletalMeshComponent* UPhysicsSubEngine::GetSkeletalMeshComponent() const
{
    return SkeletalMeshComponent;
}
