#include "FSkeletalMeshDebugger.h"

#include "Animation/AnimationPoseData.h"
#include "Engine/SkeletalMesh.h"
#include "Launch/EngineLoop.h"


void FSkeletalMeshDebugger::DrawSkeleton(const USkeletalMeshComponent* SkelComp, UPrimitiveDrawBatch* DrawBatch)
{
    if (!SkelComp || !SkelComp->GetSkeletalMeshAsset() || !SkelComp->GetSkeletalMeshAsset()->GetSkeleton())
        return;

    const USkeleton* Skeleton = SkelComp->GetSkeletalMeshAsset()->GetSkeleton();
    const FReferenceSkeleton& RefSkeleton = Skeleton->GetRefSkeleton();
    const int32 NumBones = RefSkeleton.GetRawBoneNum();

    TArray<FMatrix> BoneWorldMatrices;
    SkelComp->GetCurrentGlobalBoneMatrices(BoneWorldMatrices);

    constexpr int32 ConeSegment = 12;
    constexpr float ConeThicknessRatio = 0.1f;
    const FVector4 ConeColor(0.2f, 1.f, 0.2f, 1.f);

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        int32 ParentIndex = RefSkeleton.RawRefBoneInfo[BoneIndex].ParentIndex;
        if (ParentIndex < 0 || ParentIndex >= NumBones)
            continue;

        FVector PosChild = BoneWorldMatrices[BoneIndex].GetOrigin();
        FVector PosParent = BoneWorldMatrices[ParentIndex].GetOrigin();

        float Length = (PosParent - PosChild).Length();
        float Radius = Length * ConeThicknessRatio;

        // 수정된 함수 사용 (LookMatrix 제거됨)
        DrawBatch->AddConeToBatch(PosChild, PosParent, Radius, ConeSegment, ConeColor);
    }
}

void FSkeletalMeshDebugger::DrawSkeletonAABBs(const USkeletalMeshComponent* SkelComp, UPrimitiveDrawBatch* DrawBatch)
{
    if (!SkelComp || !SkelComp->GetSkeletalMeshAsset() || !SkelComp->GetSkeletalMeshAsset()->GetSkeleton())
        return;

    const USkeleton* Skeleton = SkelComp->GetSkeletalMeshAsset()->GetSkeleton();
    const FReferenceSkeleton& RefSkeleton = Skeleton->GetRefSkeleton();
    const int32 NumBones = RefSkeleton.GetRawBoneNum();

    TArray<FMatrix> BoneWorldMatrices;
    SkelComp->GetCurrentGlobalBoneMatrices(BoneWorldMatrices);

    float TotalLength = 0.f;
    int32 Count = 0;

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        int32 ParentIndex = RefSkeleton.RawRefBoneInfo[BoneIndex].ParentIndex;
        if (ParentIndex < 0) continue;

        FVector PosA = BoneWorldMatrices[BoneIndex].GetOrigin();
        FVector PosB = BoneWorldMatrices[ParentIndex].GetOrigin();
        TotalLength += (PosA - PosB).Length();
        ++Count;
    }

    float HalfExtent = (Count > 0 ? TotalLength / Count : 10.f) * 0.1f;
    const FVector BoxHalf = FVector(HalfExtent);
    const FVector4 BoxColor(1.f, 0.2f, 0.2f, 1.f);

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        FVector Pos = BoneWorldMatrices[BoneIndex].GetOrigin();
        FBoundingBox Box;
        Box.MinLocation = -BoxHalf;
        Box.MaxLocation = BoxHalf;
        DrawBatch->AddAABBToBatch(Box, Pos, FMatrix::Identity);
    }
}
