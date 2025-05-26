#include "FSkeletalMeshDebugger.h"

#include "Animation/AnimationPoseData.h"
#include "Engine/SkeletalMesh.h"
#include "Launch/EngineLoop.h"

//현재 Bone의 위치를 Cone의 꼭지, 부모 Bone의 위치를 Cone의 바닥으로 사용
/*void FSkeletalMeshDebugger::DrawSkeleton(const USkeletalMeshComponent* SkelComp, UPrimitiveDrawBatch* DrawBatch)
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

    const FVector4 DefaultConeColor(0.2f, 1.f, 0.2f, 1.f);     // 연녹색
    const FVector4 SelectedConeColor(1.f, 0.1f, 0.1f, 1.f);    // 빨강색

    const int32 SelectedBoneIndex = SkelComp->GetSelectedBone();

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        int32 ParentIndex = RefSkeleton.RawRefBoneInfo[BoneIndex].ParentIndex;
        if (ParentIndex < 0 || ParentIndex >= NumBones)
            continue;

        FVector PosChild = BoneWorldMatrices[BoneIndex].GetOrigin();
        FVector PosParent = BoneWorldMatrices[ParentIndex].GetOrigin();

        float Length = (PosParent - PosChild).Length();
        float Radius = Length * ConeThicknessRatio;

        const FVector4 Color = (BoneIndex == SelectedBoneIndex) ? SelectedConeColor : DefaultConeColor;

        DrawBatch->AddConeToBatch(PosChild, PosParent, Radius, ConeSegment, Color);
    }
}*/
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

    const FVector4 DefaultConeColor(0.2f, 1.f, 0.2f, 1.f);     // 연녹색
    const FVector4 SelectedConeColor(1.f, 0.1f, 0.1f, 1.f);    // 빨강색

    const int32 SelectedBoneIndex = SkelComp->GetSelectedBone();

    // ✅ 최적화: 부모 → 자식 인덱스 맵 사전 구성
    TArray<TArray<int32>> BoneChildren;
    BoneChildren.SetNum(NumBones);

    for (int32 i = 0; i < NumBones; ++i)
    {
        int32 ParentIndex = RefSkeleton.GetParentIndex(i);
        if (ParentIndex >= 0 && ParentIndex < NumBones)
        {
            BoneChildren[ParentIndex].Add(i);
        }
    }

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        const FVector BonePos = BoneWorldMatrices[BoneIndex].GetOrigin();
        const FVector4 Color = (BoneIndex == SelectedBoneIndex) ? SelectedConeColor : DefaultConeColor;

        const TArray<int32>& Children = BoneChildren[BoneIndex];
        if (Children.Num() > 0)
        {
            for (int32 ChildIndex : Children)
            {
                const FVector ChildPos = BoneWorldMatrices[ChildIndex].GetOrigin();
                float Length = (BonePos - ChildPos).Length();
                float Radius = Length * ConeThicknessRatio;

                // ✅ Cone의 Apex = 자식 위치, Base = 본 위치
                DrawBatch->AddConeToBatch(ChildPos, BonePos, Radius, ConeSegment, Color);
            }
        }
        /*else
        {
            // 자식이 없으면 기본 방향으로 짧은 Cone 렌더
            FVector TipPos = BonePos + BoneWorldMatrices[BoneIndex].GetScaledAxis(EAxis::Z) * 5.0f;
            float Length = 5.0f;
            float Radius = Length * ConeThicknessRatio;

            DrawBatch->AddConeToBatch(TipPos, BonePos, Radius, ConeSegment, Color);
        }*/
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
