#include "FSkeletalMeshDebugger.h"

#include "Animation/AnimationPoseData.h"
#include "Engine/SkeletalMesh.h"
#include "Launch/EngineLoop.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

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
//TODO
//위치는 RefSkel 기준이 아닌 현재 Bone 위치 기준으로 할지 결정
//Rot는 RefSkel로 하는게 맞음
void FSkeletalMeshDebugger::DrawConeConstraints(const USkeletalMeshComponent* SkelComp, UPrimitiveDrawBatch* DrawBatch, const FName& SelectedConstraintName)
{
    if (!SkelComp || !DrawBatch) return;

    UPhysicsAsset* PhysAsset = SkelComp->GetPhysicsAsset();
    if (!PhysAsset) return;

    const USkeleton* Skeleton = SkelComp->GetSkeletalMeshAsset()->GetSkeleton();
    const FReferenceSkeleton& RefSkeleton = Skeleton->GetRefSkeleton();

    constexpr int32 ConeSegments = 16;
    const FVector4 ConeColor(1.f, 1.f, 0.f, 1.f); // 노란색

    // [1] Constraint 기준으로 원뿔 렌더링
    for (UPhysicsConstraintTemplate* Constraint : PhysAsset->ConstraintSetup)
    {
        if (!Constraint) continue;
        const FConstraintInstance& Inst = Constraint->DefaultInstance;


        const FName& BoneName = Inst.ConstraintBone1;
        if (Inst.JointName != SelectedConstraintName&&BoneName!=SelectedConstraintName)
            continue;
        int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
        if (!RefSkeleton.IsValidRawIndex(BoneIndex)) continue;

        // ConstraintBone1의 자식 중 하나를 찾음
        int32 ChildIndex = INDEX_NONE;
        for (int32 i = 0; i < RefSkeleton.GetRawBoneNum(); ++i)
        {
            if (RefSkeleton.GetParentIndex(i) == BoneIndex)
            {
                ChildIndex = i;
                break;
            }
        }
        if (!RefSkeleton.IsValidRawIndex(ChildIndex)) continue;

        const FVector Start = RefSkeleton.GetRefWorldTransform(BoneIndex).GetTranslation();
        const FVector End = RefSkeleton.GetRefWorldTransform(ChildIndex).GetTranslation();

        const float Length = (End - Start).Length();
        if (Length < KINDA_SMALL_NUMBER) continue;

        const float AvgSwing = (Inst.ProfileInstance.ConeLimit.Swing1LimitDegrees + Inst.ProfileInstance.ConeLimit.Swing2LimitDegrees) * 0.5f;
        const float Radius = FMath::Tan(FMath::DegreesToRadians(AvgSwing)) * Length;

        DrawBatch->AddConeToBatch(Start, End, Radius, ConeSegments, ConeColor);
    }
}

void FSkeletalMeshDebugger::DrawCapsuleOBBs(const USkeletalMeshComponent* SkelComp, UPrimitiveDrawBatch* DrawBatch, const FName& SelectedBodyName)
{
    if (!SkelComp || !DrawBatch || !SkelComp->GetSkeletalMeshAsset()) return;

    const USkeleton* Skeleton = SkelComp->GetSkeletalMeshAsset()->GetSkeleton();
    if (!Skeleton) return;

    const FReferenceSkeleton& RefSkeleton = Skeleton->GetRefSkeleton();
    const UPhysicsAsset* PhysAsset = SkelComp->GetPhysicsAsset();
    if (!PhysAsset) return;

    TArray<FMatrix> BoneWorldMatrices;
    SkelComp->GetCurrentGlobalBoneMatrices(BoneWorldMatrices);

    const FVector4 DefaultColor(0.3f, 0.7f, 1.f, 1.f);  // 파란색
    const FVector4 SelectedColor(1.f, 0.2f, 0.2f, 1.f); // 빨간색

    for (int32 BodyIndex = 0; BodyIndex < PhysAsset->BodySetup.Num(); ++BodyIndex)
    {
        const UBodySetup* BodySetup = PhysAsset->BodySetup[BodyIndex];
        if (!BodySetup) continue;

        const FName& BoneName = BodySetup->BoneName;
        int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
        if (!BoneWorldMatrices.IsValidIndex(BoneIndex)) continue;

        const FMatrix& BoneMatrix = BoneWorldMatrices[BoneIndex];
        const FVector4 Color = (BoneName == SelectedBodyName) ? SelectedColor : DefaultColor;

        for (const FKSphylElem& Sphyl : BodySetup->AggGeom.SphylElems)
        {
            const FTransform LocalTransform = Sphyl.GetTransform();
            //const FTransform WorldTransform = LocalTransform * FTransform(BoneMatrix);

            const float HalfLength = Sphyl.Length * 0.5f;
            const FVector BoxExtent(Sphyl.Radius, Sphyl.Radius, HalfLength);

            FBoundingBox LocalBox;
            LocalBox.MinLocation = -BoxExtent;
            LocalBox.MaxLocation = BoxExtent;
            DrawBatch->AddOBBToBatch(LocalBox, Sphyl.Center, LocalTransform.Rotation.ToMatrix(), Color);
            //DrawBatch->AddOBBToBatch(LocalBox, WorldTransform.GetTranslation(), WorldTransform.ToMatrixNoScale(), Color);
        }
    }
}
