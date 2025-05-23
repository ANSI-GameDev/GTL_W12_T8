#include "SubRenderer.h"

#include "PhysicsSubEngine.h"
#include "ShadowManager.h"
#include "SkeletalMeshRenderPass.h"
#include "UnrealEd/EditorViewportClient.h"
#include "Engine/UnrealClient.h"
#include "Renderer/ParticleRenderPass.h"
#include "SubWindow/ParticleSubEngine.h"
void FSubRenderer::Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, USubEngine* InEngine)
{
    Engine = InEngine;
    Graphics = InGraphics;
    BufferManager = InBufferManager;
    ShadowManager = new FShadowManager();
    ShadowManager->Initialize(Graphics, BufferManager);
    //렌더패스 Init은 SetEnabledPass에서 수행
    /*ParticleRenderPass = new FParticleRenderPass();
    ParticleRenderPass->Initialize(BufferManager, Graphics, FEngineLoop::Renderer.ShaderManager);
    SkeletalMeshRenderPass = new FSkeletalMeshRenderPass();*/
}

void FSubRenderer::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    UpdateViewCamera(Viewport);
    
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    ViewportResource->ClearDepthStencils(Graphics->DeviceContext);
    ViewportResource->ClearRenderTargets(Graphics->DeviceContext);
    if (EnabledPasses["Particle"])
    {
        auto* ParticleEngine = dynamic_cast<UParticleSubEngine*>(Engine);
        if (ParticleEngine && ParticleRenderPass)
        {
            ParticleRenderPass->AddParticleComponent(ParticleEngine->GetParticleSystemComponent());
        }
    }

    if (EnabledPasses["Skeletal"])
    {
        auto* PhysicsEngine = dynamic_cast<UPhysicsSubEngine*>(Engine);
        if (PhysicsEngine&&SkeletalMeshRenderPass)
        {
            SkeletalMeshRenderPass->AddSkeletalMeshComponent(PhysicsEngine->GetSkeletalMeshComponent());
        }
    }
}
void FSubRenderer::SetEnabledPass(FString PassName, bool bEnable)
{
    if (bEnable)
    {
        if (PassName == "Particle" && !ParticleRenderPass)
        {
            ParticleRenderPass = new FParticleRenderPass();
            ParticleRenderPass->Initialize(BufferManager, Graphics, FEngineLoop::Renderer.ShaderManager);
        }
        else if (PassName == "Skeletal" && !SkeletalMeshRenderPass)
        {
            SkeletalMeshRenderPass = new FSkeletalMeshRenderPass();
            SkeletalMeshRenderPass->Initialize(BufferManager, Graphics, FEngineLoop::Renderer.ShaderManager);
            SkeletalMeshRenderPass->InitializeShadowManager(ShadowManager);

        }
        EnabledPasses.Add(PassName, true);
    }
    else
    {
        EnabledPasses.Add(PassName, false);
    }
}


void FSubRenderer::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    if (EnabledPasses["Particle"])
    {
        ParticleRenderPass->Render(Viewport);
    }

    if (EnabledPasses["Skeletal"])
    {
        SkeletalMeshRenderPass->Render(Viewport);
    }
}

void FSubRenderer::ClearRender()
{
    if (EnabledPasses["Particle"])
    {
        ParticleRenderPass->ClearRenderArr();
    }

    if (EnabledPasses["Skeletal"])
    {
        SkeletalMeshRenderPass->ClearRenderArr();
    }
}

void FSubRenderer::Release()
{
    if (ParticleRenderPass)
    {
        delete ParticleRenderPass;
        ParticleRenderPass = nullptr;
    }

    if (SkeletalMeshRenderPass)
    {
        delete SkeletalMeshRenderPass;
        SkeletalMeshRenderPass = nullptr;
    }

    if (ShadowManager)
    {
        delete ShadowManager;
        ShadowManager = nullptr;
    }
}

void FSubRenderer::UpdateViewCamera(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FCameraConstantBuffer CameraConstantBuffer;
    CameraConstantBuffer.ViewMatrix = Viewport->GetViewMatrix();
    CameraConstantBuffer.InvViewMatrix = FMatrix::Inverse(CameraConstantBuffer.ViewMatrix);
    CameraConstantBuffer.ProjectionMatrix = Viewport->GetProjectionMatrix();
    CameraConstantBuffer.InvProjectionMatrix = FMatrix::Inverse(CameraConstantBuffer.ProjectionMatrix);
    CameraConstantBuffer.ViewLocation = Viewport->GetCameraLocation();
    CameraConstantBuffer.NearClip = Viewport->GetCameraNearClip();
    CameraConstantBuffer.FarClip = Viewport->GetCameraFarClip();
    BufferManager->UpdateConstantBuffer("FCameraConstantBuffer", CameraConstantBuffer);
}
