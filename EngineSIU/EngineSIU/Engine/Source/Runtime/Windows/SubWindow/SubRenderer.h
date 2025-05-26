#pragma once
#include <memory>

#include "Container/Map.h"
class FCompositingPass;
class FLineRenderPass;
class UPrimitiveDrawBatch;
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

    FLineRenderPass* GetLineRenderPass() const { return LineRenderPass; }
    UPrimitiveDrawBatch* PrimitiveDrawBatch = nullptr;
private:
    void UpdateViewCamera(const std::shared_ptr<FEditorViewportClient>& Viewport);
    USubEngine* Engine = nullptr;
    FGraphicsDevice* Graphics=nullptr;
    FDXDBufferManager* BufferManager=nullptr;
    FShadowManager* ShadowManager = nullptr;
    FParticleRenderPass* ParticleRenderPass = nullptr;
    FSkeletalMeshRenderPass* SkeletalMeshRenderPass = nullptr;

    FLineRenderPass* LineRenderPass = nullptr;
    FCompositingPass* CompositingPass = nullptr;

    //Line, Compositing의 경우 공통으로 사용되므로 제외
    TMap<FString, bool> EnabledPasses;
};

