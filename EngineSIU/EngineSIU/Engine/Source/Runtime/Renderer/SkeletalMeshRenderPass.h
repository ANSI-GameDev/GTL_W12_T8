#pragma once
#include "SkeletalMeshRenderPassBase.h"

enum class EViewModeIndex : uint8;

class FSkeletalMeshRenderPass : public FSkeletalMeshRenderPassBase
{
public: 
    void AddSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent);
protected:
    virtual void CreateResource();

    virtual void ReleaseResource();
    
    virtual void PrepareRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport);

    virtual void CleanUpRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void ChangeViewMode(EViewModeIndex ViewMode);

    void UpdateLitUnlitConstant(int32 IsLit) const;
};
