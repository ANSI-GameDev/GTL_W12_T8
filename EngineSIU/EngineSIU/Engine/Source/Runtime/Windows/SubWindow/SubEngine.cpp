#include "SubEngine.h"

#include "ImGuiManager.h"
#include "ImGuiSubWindow.h"
#include "ParticleRenderPass.h"
#include "ParticleSubEngine.h"
#include "SubRenderer.h"
#include "UnrealClient.h"
#include "UnrealEd/EditorViewportClient.h"

USubEngine::USubEngine() :
    Graphics(nullptr),
    BufferManager(nullptr),
    Wnd(nullptr),
    SubRenderer(nullptr),
    UnrealEditor(nullptr),
    SubUI(nullptr),
    bIsShowSubWindow(false),
    EditorPlayer(nullptr)
{
}

USubEngine::~USubEngine()
{
}

void USubEngine::Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow, UnrealEd* InUnrealEd)
{
    Graphics = InGraphics;
    BufferManager = InBufferManager;
    Wnd = &hWnd;
    SubRenderer = new FSubRenderer;
    UnrealEditor = InUnrealEd;
    SubUI = new FImGuiSubWindow(hWnd, InGraphics->Device, InGraphics->DeviceContext);
    UImGuiManager::ApplySharedStyle(InSubWindow->GetContext(), SubUI->Context);
    SubRenderer->Initialize(InGraphics, InBufferManager, this);

    ViewportClient = std::make_shared<FEditorViewportClient>();
    ViewportClient->Initialize(EViewScreenLocation::EVL_MAX, FRect(0, 0, 800, 600), this);
    ViewportClient->CameraReset();
    ViewportClient->FarClip = 1000000;
    ViewportClient->ViewFOV = 60.f;
    ViewportClient->SetCameraSpeed(5.0f);

}



void USubEngine::Input(float DeltaTime)
{

}

void USubEngine::Tick(float DeltaTime)
{
}

void USubEngine::Render()
{

}

void USubEngine::Release()
{
    if (SubRenderer)
    {
        SubRenderer->Release();
        delete SubRenderer;
        SubRenderer = nullptr;
    }
    if (SubUI)
    {
        SubUI->Shutdown();
        delete SubUI;
        SubUI = nullptr;
    }
}

void USubEngine::RequestShowWindow(bool bShow)
{
    bIsShowSubWindow = bShow;
    bIsShowing = bShow;
}
