#pragma once
#include "UnrealEd/EditorPanel.h"
#include "UnrealEd/EditorViewportClient.h"
#include <Windows.h> // HWND 정의를 위해 추가

#include "Misc/EnumClassFlags.h"

struct FBaseCompactPose;
struct FTransform;
struct FReferenceSkeleton;
class USkeletalMeshComponent;
struct FRenderTargetRHI;
struct FDepthStencilRHI;

class FEditorViewportClient;
enum class EPhysicsDebugDisplay : uint32
{
    None = 0,
    Bone = 1 << 0,
    Body = 1 << 1,
    BodySelectedOnly = 1 << 2,
    Constraint = 1 << 3,
    ConstraintSelectedOnly = 1 << 4,
};
ENUM_CLASS_FLAGS(EPhysicsDebugDisplay)
enum class EPhysicsSelectionType : uint8
{
    None,
    Bone,
    Body,
    Constraint
};

class PhysicsViewerPanel : public UEditorPanel
{
public:
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override; // 시그니처 유지
public:
    void SetViewportClient(std::shared_ptr<FEditorViewportClient> InViewportClient);
    void SetSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent);
    void SetPrimitiveDrawBatch(UPrimitiveDrawBatch* InPrimitiveDrawBatch);
private:
    void RenderViewportPanel();
    void RenderPhysicsSettings();
    void RenderInfoPanel();
    void RenderBoneRecursive(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, FBaseCompactPose& Pose);
    void RenderSelectedProperty(FBaseCompactPose& Pose);

    void RenderPanelLayout();
    void RenderSkeletonUI();
    void DrawShowFlags();
    void ToggleFlag(EPhysicsDebugDisplay Flag, bool bEnable);
public:
    UPrimitiveDrawBatch* PrimitiveDrawBatch = nullptr;
private:
    float Width = 800.0f;
    float Height = 600.0f;
    EPhysicsDebugDisplay DebugDisplayFlags =
        EPhysicsDebugDisplay::Bone |
        EPhysicsDebugDisplay::Body |
        EPhysicsDebugDisplay::BodySelectedOnly| 
        EPhysicsDebugDisplay::ConstraintSelectedOnly;
    
    std::shared_ptr<FEditorViewportClient> ViewportClient;
    USkeletalMeshComponent* SkeletalMeshComponent = nullptr;

    EPhysicsSelectionType SelectedType = EPhysicsSelectionType::None;
    FName SelectedName;
    //int SelectedBoneIndex = -1;
};
