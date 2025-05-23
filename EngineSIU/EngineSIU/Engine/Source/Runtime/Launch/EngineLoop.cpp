#include "EngineLoop.h"
#include "ImGuiManager.h"
#include "UnrealClient.h"
#include "WindowsPlatformTime.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Engine/EditorEngine.h"
#include "LevelEditor/SLevelEditor.h"
#include "PropertyEditor/ViewportTypePanel.h"
#include "Slate/Widgets/Layout/SSplitter.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealEd/UnrealEd.h"
#include "World/World.h"

#include "Engine/EditorEngine.h"
#include "Renderer/DepthPrePass.h"
#include "Renderer/TileLightCullingPass.h"
#include "SubWindow/ParticleSubEngine.h"
#include "SubWindow/ImGuiSubWindow.h"
#include "SoundManager.h"
#include "SubWindow/PhysicsSubEngine.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

FGraphicsDevice FEngineLoop::GraphicDevice;
FGraphicsDevice FEngineLoop::ParticleViewerGD;
FGraphicsDevice FEngineLoop::PhysicsViewerGD;

FRenderer FEngineLoop::Renderer;
UPrimitiveDrawBatch FEngineLoop::PrimitiveDrawBatch;
FResourceManager FEngineLoop::ResourceManager;
uint32 FEngineLoop::TotalAllocationBytes = 0;
uint32 FEngineLoop::TotalAllocationCount = 0;

FEngineLoop::FEngineLoop()
    : AppWnd(nullptr)
    , UIManager(nullptr)
    , LevelEditor(nullptr)
    , UnrealEditor(nullptr)
    , BufferManager(nullptr)
{
}

int32 FEngineLoop::PreInit()
{
    return 0;
}

int32 FEngineLoop::Init(HINSTANCE hInstance)
{
    FPlatformTime::InitTiming();

    /* must be initialized before window. */
    WindowInit(hInstance);

    UnrealEditor = new UnrealEd();
    BufferManager = new FDXDBufferManager();
    UIManager = new UImGuiManager;
    AppMessageHandler = std::make_unique<FSlateAppMessageHandler>();
    LevelEditor = new SLevelEditor();

    UnrealEditor->Initialize();
    GraphicDevice.Initialize(AppWnd);

    if (!GPUTimingManager.Initialize(GraphicDevice.Device, GraphicDevice.DeviceContext))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to initialize GPU Timing Manager!"));
    }
    EngineProfiler.SetGPUTimingManager(&GPUTimingManager);

    // @todo Table에 Tree 구조로 넣을 수 있도록 수정
    EngineProfiler.RegisterStatScope(TEXT("Renderer_Render"), FName(TEXT("Renderer_Render_CPU")), FName(TEXT("Renderer_Render_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- DepthPrePass"), FName(TEXT("DepthPrePass_CPU")), FName(TEXT("DepthPrePass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- TileLightCulling"), FName(TEXT("TileLightCulling_CPU")), FName(TEXT("TileLightCulling_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- ShadowPass"), FName(TEXT("ShadowPass_CPU")), FName(TEXT("ShadowPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- StaticMeshPass"), FName(TEXT("StaticMeshPass_CPU")), FName(TEXT("StaticMeshPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- WorldBillboardPass"), FName(TEXT("WorldBillboardPass_CPU")), FName(TEXT("WorldBillboardPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- UpdateLightBufferPass"), FName(TEXT("UpdateLightBufferPass_CPU")), FName(TEXT("UpdateLightBufferPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- FogPass"), FName(TEXT("FogPass_CPU")), FName(TEXT("FogPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- PostProcessCompositing"), FName(TEXT("PostProcessCompositing_CPU")), FName(TEXT("PostProcessCompositing_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- EditorBillboardPass"), FName(TEXT("EditorBillboardPass_CPU")), FName(TEXT("EditorBillboardPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- EditorRenderPass"), FName(TEXT("EditorRenderPass_CPU")), FName(TEXT("EditorRenderPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- LinePass"), FName(TEXT("LinePass_CPU")), FName(TEXT("LinePass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- GizmoPass"), FName(TEXT("GizmoPass_CPU")), FName(TEXT("GizmoPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- CompositingPass"), FName(TEXT("CompositingPass_CPU")), FName(TEXT("CompositingPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- SkinningPass"), FName(TEXT("SkinningPass_CPU")), FName(TEXT("SkinningPass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("|- ParticlePass"), FName(TEXT("ParticlePass_CPU")), FName(TEXT("ParticlePass_GPU")));
    EngineProfiler.RegisterStatScope(TEXT("SlatePass"), FName(TEXT("SlatePass_CPU")), FName(TEXT("SlatePass_GPU")));

    BufferManager->Initialize(GraphicDevice.Device, GraphicDevice.DeviceContext);
    Renderer.Initialize(&GraphicDevice, BufferManager, &GPUTimingManager);
    PrimitiveDrawBatch.Initialize(&GraphicDevice);
    UIManager->Initialize(AppWnd, GraphicDevice.Device, GraphicDevice.DeviceContext);
    ResourceManager.Initialize(&Renderer, &GraphicDevice);
    
    uint32 ClientWidth = 0;
    uint32 ClientHeight = 0;
    GetClientSize(ClientWidth, ClientHeight);
    LevelEditor->Initialize(ClientWidth, ClientHeight);

    GEngine = FObjectFactory::ConstructObject<UEditorEngine>(nullptr);
    GEngine->Init();


    ParticleSubWindowInit(hInstance);
    if (ParticleViewerWnd)
    {
        ParticleViewerGD.Initialize(ParticleViewerWnd, GraphicDevice.Device);
        ParticleViewerGD.ClearColor[0] = 0.03f;
        ParticleViewerGD.ClearColor[1] = 0.03f;
        ParticleViewerGD.ClearColor[2] = 0.03f;
    }

    ParticleSubEngine = FObjectFactory::ConstructObject<UParticleSubEngine>(nullptr);
    ParticleSubEngine->Initialize(ParticleViewerWnd, &ParticleViewerGD, BufferManager, UIManager, UnrealEditor);

    PhysicsSubWindowInit(hInstance);
    if (PhysicsViewerWnd)
    {
        PhysicsViewerGD.Initialize(PhysicsViewerWnd, GraphicDevice.Device);
        PhysicsViewerGD.ClearColor[0] = 0.03f;
        PhysicsViewerGD.ClearColor[1] = 0.03f;
        PhysicsViewerGD.ClearColor[2] = 0.03f;
    }

    PhysicsSubEngine = FObjectFactory::ConstructObject<UPhysicsSubEngine>(nullptr);
    PhysicsSubEngine->Initialize(PhysicsViewerWnd, &PhysicsViewerGD, BufferManager, UIManager, UnrealEditor);

    FSoundManager::GetInstance().Initialize();
    FSoundManager::GetInstance().LoadSound("fishdream", "Contents/Sounds/fishdream.mp3");
    FSoundManager::GetInstance().LoadSound("sizzle", "Contents/Sounds/sizzle.mp3");
    //FSoundManager::GetInstance().PlaySound("fishdream");

    UpdateUI();

    return 0;
}

void FEngineLoop::Render() const
{
    GraphicDevice.Prepare();
    
    if (LevelEditor->IsMultiViewport())
    {
        const std::shared_ptr<FEditorViewportClient> ActiveViewportCache = GetLevelEditor()->GetActiveViewportClient();
        for (int Idx = 0; Idx < 4; ++Idx)
        {
            LevelEditor->SetActiveViewportClient(Idx);
            Renderer.Render(LevelEditor->GetActiveViewportClient());
        }
        
        for (int Idx = 0; Idx < 4; ++Idx)
        {
            LevelEditor->SetActiveViewportClient(Idx);
            Renderer.RenderViewport(LevelEditor->GetActiveViewportClient());
        }
        GetLevelEditor()->SetActiveViewportClient(ActiveViewportCache);
    }
    else
    {
        Renderer.Render(LevelEditor->GetActiveViewportClient());
        
        Renderer.RenderViewport(LevelEditor->GetActiveViewportClient());
    }
    
}

void FEngineLoop::Tick()
{
    LARGE_INTEGER Frequency;
    const double TargetFrameTime = 1000.0 / TargetFPS; // 한 프레임의 목표 시간 (밀리초 단위)

    QueryPerformanceFrequency(&Frequency);

    LARGE_INTEGER StartTime, EndTime;
    double ElapsedTime = 0.0;

    while (bIsExit == false)
    {
        FProfilerStatsManager::BeginFrame();    // Clear previous frame stats
        if (GPUTimingManager.IsInitialized())
        {
            GPUTimingManager.BeginFrame();      // Start GPU frame timing
        }

        QueryPerformanceCounter(&StartTime);

        MSG Msg;
        while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Msg); // 키보드 입력 메시지를 문자메시지로 변경
            DispatchMessage(&Msg);  // 메시지를 WndProc에 전달

            if (Msg.message == WM_QUIT)
            {
                bIsExit = true;
                break;
            }
        }

        if (!bIsExit && ParticleViewerWnd && IsWindowVisible(ParticleViewerWnd))
        {
            while (PeekMessage(&Msg, ParticleViewerWnd, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
        if (!bIsExit && PhysicsViewerWnd && IsWindowVisible(PhysicsViewerWnd))
        {
            while (PeekMessage(&Msg, PhysicsViewerWnd, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }


        // Engine loop Break
        if (bIsExit) break;

        const float DeltaTime = static_cast<float>(ElapsedTime / 1000.f);

        GEngine->Tick(DeltaTime);
        LevelEditor->Tick(DeltaTime);

        Render();

        UIManager->BeginFrame();
        UnrealEditor->Render();

        FConsole::GetInstance().Draw();
        EngineProfiler.Render(GraphicDevice.DeviceContext, GraphicDevice.ScreenWidth, GraphicDevice.ScreenHeight);

        UIManager->EndFrame();

        if (ParticleSubEngine->bIsShowing)
            ParticleSubEngine->Tick(DeltaTime);

        if (PhysicsSubEngine->bIsShowing)
            PhysicsSubEngine->Tick(DeltaTime);
        if (CurrentImGuiContext != nullptr)
        {
            ImGui::SetCurrentContext(CurrentImGuiContext);
        }

        // Pending 처리된 오브젝트 제거
        GUObjectArray.ProcessPendingDestroyObjects();

        if (GPUTimingManager.IsInitialized())
        {
            GPUTimingManager.EndFrame();        // End GPU frame timing
        }

        GraphicDevice.SwapBuffer();

        SubEngineControl();
        do
        {
            Sleep(0);
            QueryPerformanceCounter(&EndTime);
            ElapsedTime = (static_cast<double>(EndTime.QuadPart - StartTime.QuadPart) * 1000.f / static_cast<double>(Frequency.QuadPart));
        } while (ElapsedTime < TargetFrameTime);
    }
    FSoundManager::GetInstance().Update();
}

void FEngineLoop::GetClientSize(uint32& OutWidth, uint32& OutHeight) const
{
    RECT ClientRect = {};
    GetClientRect(AppWnd, &ClientRect);
            
    OutWidth = ClientRect.right - ClientRect.left;
    OutHeight = ClientRect.bottom - ClientRect.top;
}

void FEngineLoop::OpenParticleSystemViewer()
{
    if (ParticleSubEngine->bIsShowSubWindow)
    {
        if (ParticleViewerWnd)
        {
            ::ShowWindow(ParticleViewerWnd, SW_SHOW);
        }
        ParticleSubEngine->bIsShowSubWindow = false;
    }
}
void FEngineLoop::OpenPhysicsViewer()
{
    if (PhysicsSubEngine->bIsShowSubWindow)
    {
        if (PhysicsViewerWnd)
        {
            ::ShowWindow(PhysicsViewerWnd, SW_SHOW);
        }
        PhysicsSubEngine->bIsShowSubWindow = false;
    }
}

void FEngineLoop::SubEngineControl()
{
    OpenParticleSystemViewer();
    OpenPhysicsViewer();;
}

void FEngineLoop::Exit()
{

    ParticleSubEngine->Release();
    CleanupSubWindow();

    LevelEditor->Release();
    UIManager->Shutdown();
    ResourceManager.Release(&Renderer);
    Renderer.Release();
    GraphicDevice.Release();
    
    GEngine->Release();

    delete UnrealEditor;
    delete BufferManager;
    delete UIManager;
    delete LevelEditor;
}

void FEngineLoop::WindowInit(HINSTANCE hInstance)
{
    WCHAR WindowClass[] = L"JungleWindowClass";

    WCHAR Title[] = L"Game Tech Lab";

    WNDCLASSW Wc{};
    Wc.lpfnWndProc = AppWndProc;
    Wc.hInstance = hInstance;
    Wc.lpszClassName = WindowClass;
    Wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;

    RegisterClassW(&Wc);

    AppWnd = CreateWindowExW(
        0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1400, 1000,
        nullptr, nullptr, hInstance, nullptr
    );
}

LRESULT CALLBACK FEngineLoop::AppWndProc(HWND hWnd, uint32 Msg, WPARAM wParam, LPARAM lParam)
{
    if (hWnd == GEngineLoop.AppWnd)
    {
        ImGui::SetCurrentContext(GEngineLoop.UIManager->GetContext());
        if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam)) return true;
    }
    else if (hWnd == GEngineLoop.ParticleViewerWnd)
    {
        ImGui::SetCurrentContext(GEngineLoop.ParticleSubEngine->SubUI->Context);

        if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam)) return true;

        /** SubWindow Msg */
        switch (Msg)
        {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                RECT ClientRect;
                GetClientRect(hWnd, &ClientRect);

                float FullWidth = static_cast<float>(ClientRect.right - ClientRect.left);
                float FullHeight = static_cast<float>(ClientRect.bottom - ClientRect.top);

                if (GEngineLoop.GetUnrealEditor())
                {
                    ParticleViewerGD.Resize(hWnd, FullWidth, FullHeight);
                    GEngineLoop.GetUnrealEditor()->OnResize(hWnd, EWindowType::WT_ParticleSubWindow);
                }
                GEngineLoop.ParticleSubEngine->ViewportClient->AspectRatio = (FullWidth * 0.75f) / FullHeight;
            }
            return 0;
        case WM_CLOSE:
            GEngineLoop.ParticleSubEngine->ViewportClient->CameraReset();
            GEngineLoop.ParticleSubEngine->RequestShowWindow(false);
            ::ShowWindow(hWnd, SW_HIDE);
            return 0;

        case WM_ACTIVATE:
            if (ImGui::GetCurrentContext() == nullptr) break;
            ImGui::SetCurrentContext(GEngineLoop.ParticleSubEngine->SubUI->Context);
            GEngineLoop.CurrentImGuiContext = ImGui::GetCurrentContext();
            return 0;
        case WM_KEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            ::SetFocus(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
        }
    }
    else if (hWnd == GEngineLoop.PhysicsViewerWnd)
    {
        ImGui::SetCurrentContext(GEngineLoop.PhysicsSubEngine->SubUI->Context);
        if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam)) return true;

        switch (Msg)
        {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                RECT ClientRect;
                GetClientRect(hWnd, &ClientRect);

                float FullWidth = static_cast<float>(ClientRect.right - ClientRect.left);
                float FullHeight = static_cast<float>(ClientRect.bottom - ClientRect.top);

                GEngineLoop.PhysicsViewerGD.Resize(hWnd, FullWidth, FullHeight);

                if (GEngineLoop.GetUnrealEditor())
                {
                    GEngineLoop.GetUnrealEditor()->OnResize(hWnd, EWindowType::WT_PhysicsSubWindow);
                }

                if (GEngineLoop.PhysicsSubEngine->ViewportClient)
                {
                    GEngineLoop.PhysicsSubEngine->ViewportClient->AspectRatio = (FullWidth * 0.75f) / FullHeight;
                }
            }
            return 0;
        case WM_CLOSE:
            GEngineLoop.PhysicsSubEngine->RequestShowWindow(false);
            ::ShowWindow(hWnd, SW_HIDE);
            return 0;
        case WM_ACTIVATE:
            ImGui::SetCurrentContext(GEngineLoop.PhysicsSubEngine->SubUI->Context);
            GEngineLoop.CurrentImGuiContext = ImGui::GetCurrentContext();
            return 0;
        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
        }
    }


    //if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam))
    //{
    //    return true;
    //}

    switch (Msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        if (auto LevelEditor = GEngineLoop.GetLevelEditor())
        {
            LevelEditor->SaveConfig();
        }
        GEngineLoop.bIsExit = true;
        break;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            if (auto LevelEditor = GEngineLoop.GetLevelEditor())
            {
                FEngineLoop::GraphicDevice.Resize(hWnd);
                // FEngineLoop::Renderer.DepthPrePass->ResizeDepthStencil();
                
                uint32 ClientWidth = 0;
                uint32 ClientHeight = 0;
                GEngineLoop.GetClientSize(ClientWidth, ClientHeight);
            
                LevelEditor->ResizeEditor(ClientWidth, ClientHeight);
                FEngineLoop::Renderer.TileLightCullingPass->ResizeViewBuffers(
                  static_cast<uint32>(LevelEditor->GetActiveViewportClient()->GetD3DViewport().Width),
                    static_cast<uint32>(LevelEditor->GetActiveViewportClient()->GetD3DViewport().Height)
                );
            }
        }
        GEngineLoop.UpdateUI();
        break;
    case WM_ACTIVATE:
        if (ImGui::GetCurrentContext() == nullptr) break;
        ImGui::SetCurrentContext(GEngineLoop.UIManager->GetContext());
        GEngineLoop.CurrentImGuiContext = ImGui::GetCurrentContext();
        break;
    default:
        if(hWnd == GEngineLoop.AppWnd && GEngineLoop.AppMessageHandler !=nullptr)
            GEngineLoop.AppMessageHandler->ProcessMessage(hWnd, Msg, wParam, lParam);
        return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}

void FEngineLoop::UpdateUI()
{
    FConsole::GetInstance().OnResize(AppWnd);
    if (GEngineLoop.GetUnrealEditor())
    {
        GEngineLoop.GetUnrealEditor()->OnResize(AppWnd);
    }
    ViewportTypePanel::GetInstance().OnResize(AppWnd);
}

void FEngineLoop::ParticleSubWindowInit(HINSTANCE hInstance)
{
    WCHAR SubWindowClass[] = L"ParticleWindowClass";
    WCHAR SubTitle[] = L"Viewer";

    WNDCLASSEXW wcexSub = {}; // WNDCLASSEXW 사용 권장
    wcexSub.cbSize = sizeof(WNDCLASSEX);
    wcexSub.style = CS_HREDRAW | CS_VREDRAW; // | CS_DBLCLKS 등 필요시 추가
    wcexSub.lpfnWndProc = AppWndProc; // 서브 윈도우 프로시저 지정
    wcexSub.cbClsExtra = 0;
    wcexSub.cbWndExtra = 0;
    wcexSub.hInstance = hInstance;
    wcexSub.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcexSub.lpszMenuName = nullptr;
    wcexSub.lpszClassName = SubWindowClass;
    //wcexSub.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
    if (!RegisterClassExW(&wcexSub))
    {
        // 오류 처리
        UE_LOG(ELogLevel::Error, TEXT("Failed to register sub window class!"));
        return;
    }

    // 서브 윈도우 생성 (크기, 위치, 스타일 조정 필요)
    // WS_OVERLAPPEDWINDOW 는 타이틀 바, 메뉴, 크기 조절 등이 포함된 일반적인 창
    // WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME 등으로 커스텀 가능
    ParticleViewerWnd = CreateWindowExW(
        0, SubWindowClass, SubTitle, WS_OVERLAPPEDWINDOW, // WS_VISIBLE 제거 (초기에는 숨김)
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, // 원하는 크기
        nullptr, // 부모 윈도우를 메인 윈도우로 설정 (선택 사항)
        nullptr, hInstance, nullptr
    );

    if (!ParticleViewerWnd)
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create sub window!"));
    }
    else
    {

    }
}
void FEngineLoop::PhysicsSubWindowInit(HINSTANCE hInstance)
{
    WCHAR SubWindowClass[] = L"PhysicsWindowClass";
    WCHAR SubTitle[] = L"Physics Viewer";

    WNDCLASSEXW wcexSub = {};
    wcexSub.cbSize = sizeof(WNDCLASSEX);
    wcexSub.lpfnWndProc = AppWndProc;
    wcexSub.hInstance = hInstance;
    wcexSub.lpszClassName = SubWindowClass;
    RegisterClassExW(&wcexSub);

    PhysicsViewerWnd = CreateWindowExW(0, SubWindowClass, SubTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
}

void FEngineLoop::CleanupSubWindow()
{
    if (ParticleViewerGD.Device)
        ParticleViewerGD.Release();
}
