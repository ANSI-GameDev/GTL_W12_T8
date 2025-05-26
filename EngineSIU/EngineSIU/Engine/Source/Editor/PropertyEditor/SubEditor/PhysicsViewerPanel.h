#pragma once
#include "UnrealEd/EditorPanel.h"
#include "UnrealEd/EditorViewportClient.h"
#include <Windows.h> // HWND 정의를 위해 추가

struct FBaseCompactPose;
struct FTransform;
struct FReferenceSkeleton;
class USkeletalMeshComponent;
struct FRenderTargetRHI;
struct FDepthStencilRHI;

class FEditorViewportClient;
enum class EPhysicsDebugDisplay : uint8
{
    None = 0,
    Bone = 1 << 0,
    Body = 1 << 1,
    Constraint = 1 << 2,
    All = Bone | Body | Constraint
};

class PhysicsViewerPanel : public UEditorPanel
{
public:
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override; // 시그니처 유지
public:
    void SetViewportClient(std::shared_ptr<FEditorViewportClient> InViewportClient);
    void SetSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent);
private:
    void RenderViewportPanel();
    void RenderPhysicsSettings();
    void RenderInfoPanel();
    void RenderBoneRecursive(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, FBaseCompactPose& Pose);

    void RenderPanelLayout();
    void RenderSkeletonUI();
private:
    float Width = 800.0f;
    float Height = 600.0f;
    EPhysicsDebugDisplay DebugDisplayFlags = EPhysicsDebugDisplay::All;
    std::shared_ptr<FEditorViewportClient> ViewportClient;
    USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
    //int SelectedBoneIndex = -1;
};
