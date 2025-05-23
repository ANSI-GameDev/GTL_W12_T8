#pragma once
#include <memory>

#include "Container/Map.h"
class FShadowManager;
class FSkeletalMeshRenderPass;
class FString;
class FGraphicsDevice;
class USubEngine;
class FDXDBufferManager;
class FEditorViewportClient;
class FParticleRenderPass;
class FSubRenderer
{
public:

    void Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, USubEngine* InEngine);
    void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void Render(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void ClearRender();
    void Release();


    void SetEnabledPass(FString PassName, bool bEnabled);
private:
    void UpdateViewCamera(const std::shared_ptr<FEditorViewportClient>& Viewport);
    USubEngine* Engine = nullptr;
    FGraphicsDevice* Graphics=nullptr;
    FDXDBufferManager* BufferManager=nullptr;
    FShadowManager* ShadowManager = nullptr;
    FParticleRenderPass* ParticleRenderPass = nullptr;
    FSkeletalMeshRenderPass* SkeletalMeshRenderPass = nullptr;
    TMap<FString, bool> EnabledPasses;
};

