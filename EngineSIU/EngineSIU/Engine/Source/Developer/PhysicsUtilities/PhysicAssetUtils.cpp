#include "PhysicAssetUtils.h"

#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "UObject/ObjectFactory.h"

#define Cal_AABB_From_BonePos true

namespace FPhysicsAssetUtils
{
    bool CreateCollisionFromBoneInternal(UBodySetup* bs, USkeletalMesh* skelMesh, int32 BoneIndex);


    bool CreateFromSkeletalMesh(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkelMesh)
    {
        if (PhysicsAsset == nullptr || SkelMesh == nullptr)
        {
            UE_LOG(ELogLevel::Error, TEXT("FPhysicsAssetUtils::CreateFromSkeletalMesh : PhysicsAsset or SkelMesh is nullptr"));
            return false;
        }

        const USkeleton* Skeleton = SkelMesh->GetSkeleton();
        if (!Skeleton)
        {
            UE_LOG(ELogLevel::Error, TEXT("FPhysicsAssetUtils::CreateFromSkeletalMesh : SkeletalMesh has no skeleton"));
            return false;
        }

        const FReferenceSkeleton& RefSkeleton = Skeleton->GetRefSkeleton();
        const int32 BoneCount = RefSkeleton.GetRawBoneNum();

        /* RefSkeleton에 대한 모든 UBodySetups (PhysAsset->BodySetup) 생성 */
        for (int32 BoneIndex = 0; BoneIndex < BoneCount; ++BoneIndex)
        {
            const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);

            const int32 NewBodyIndex = CreateNewBody(PhysicsAsset, BoneName);
            UBodySetup* NewBodySetup = PhysicsAsset->BodySetup[NewBodyIndex];

            CreateCollisionFromBoneInternal(NewBodySetup, SkelMesh, BoneIndex);
        }

        /* RefSkeleton에 대한 모든 UPhysicsConstraintTemplates (PhysAsset->ConstraintSetup) 생성 */
        for (int32 BoneIndex = 0; BoneIndex < BoneCount; ++BoneIndex)
        {
            const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
            if (ParentIndex != INDEX_NONE)
            {
                const FName ChildName = RefSkeleton.GetBoneName(BoneIndex);
                const FName ParentName = RefSkeleton.GetBoneName(ParentIndex);

                /* "pelvis_abdomen" : pelvis와 abdomen을 잇는 Joint 이름*/
                const FName ConstraintName = FName(*FString::Printf(TEXT("%s_%s_Constraint"), *ChildName.ToString(), *ParentName.ToString()));

                UPhysicsConstraintTemplate* CS = nullptr;
                int32 NewConstraintIndex = CreateNewConstraint(PhysicsAsset, ConstraintName, CS, ParentName, ChildName);



                CS->DefaultInstance.SnapTransformsToDefault(EConstraintTransformComponentFlags::All, PhysicsAsset);
                CS->SetDefaultProfile(CS->DefaultInstance);
            }
        }

        PhysicsAsset->PreviewSkeletalMesh = SkelMesh;

        return true;
    }

    int32 CreateNewConstraint(UPhysicsAsset* PhysAsset, FName InConstraintName, UPhysicsConstraintTemplate* InConstraintSetup, FName ParentBone, FName ChildBone)
    {
        if (!PhysAsset)
        {
            UE_LOG(ELogLevel::Error, TEXT("FPhysicsAssetUtils::CreateNewConstraint : PhysAsset is nullptr"));
            return INDEX_NONE;
        }

        int32 ConstraintIndex = PhysAsset->FindConstraintIndex(InConstraintName);
        if (ConstraintIndex != INDEX_NONE)
        {
            return ConstraintIndex; // Constraint가 이미 존재하면 재사용ㄷ
        }


        UPhysicsConstraintTemplate* NewConstraintSetup = FObjectFactory::ConstructObject<UPhysicsConstraintTemplate>(PhysAsset);
        if (InConstraintSetup)
        {
            NewConstraintSetup->DefaultInstance.CopyConstraintParamsFrom(&InConstraintSetup->DefaultInstance);
        }

        FConstraintInstance& Inst = NewConstraintSetup->DefaultInstance;
        Inst.JointName = InConstraintName;
        Inst.ConstraintBone1 = ChildBone;
        Inst.ConstraintBone2 = ParentBone;

        Inst.ProfileInstance.TwistLimit.TwistMotion = ACM_Limited;
        Inst.ProfileInstance.ConeLimit.Swing1Motion = ACM_Limited;
        Inst.ProfileInstance.ConeLimit.Swing2Motion = ACM_Limited;

        int32 ConstraintSetupIndex = PhysAsset->ConstraintSetup.Add(NewConstraintSetup);

        return ConstraintSetupIndex;
    }


    void DestroyConstraint(UPhysicsAsset* PhysAsset, int32 ConstraintIndex)
    {
        if (PhysAsset == nullptr)
        {
            UE_LOG(ELogLevel::Error, TEXT("FPhysicsAssetUtils::DestroyConstraint : PhysAsset is nullptr"));
            return;
        }
        PhysAsset->ConstraintSetup.RemoveAt(ConstraintIndex);
    }


    int32 CreateNewBody(UPhysicsAsset* PhysAsset, FName InBoneName)
    {
        if (PhysAsset == nullptr)
        {
            UE_LOG(ELogLevel::Error, TEXT("FPhysicsAssetUtils::CreateNewBody : PhysAsset is nullptr"));
            return INDEX_NONE;
        }

        int32 BodyIndex = PhysAsset->FindBodyIndex(InBoneName);
        if (BodyIndex != INDEX_NONE)
        {
            return BodyIndex; // 이미 있으면 재사용
        }

        UBodySetup* NewBodySetup = FObjectFactory::ConstructObject<UBodySetup>(PhysAsset);
        NewBodySetup->BoneName = InBoneName;
        NewBodySetup->bConsiderForBounds = true;    // Bounds 계산에 포함
        NewBodySetup->bDoubleSidedGeometry = true;  // 양면 충돌 허용
        NewBodySetup->BuildScale = FVector(1.0f);

        int32 BodySetupIndex = PhysAsset->BodySetup.Add(NewBodySetup);
        NewBodySetup->BoneName = InBoneName;

        PhysAsset->UpdateBodySetupIndexMap();
        PhysAsset->UpdateBoundsBodiesArray();

        return BodySetupIndex;
    }

    bool CreateCollisionFromBoneInternal(UBodySetup* bs, USkeletalMesh* skelMesh, int32 BoneIndex)
    {
        if (!bs || !skelMesh || !skelMesh->GetSkeleton()->GetRefSkeleton().IsValidRawIndex(BoneIndex))
        {
            UE_LOG(ELogLevel::Error, TEXT("FPhysicsAssetUtils::CreateCollisionFromBoneInternal : Invalid parameters"));
            return false;
        }

        const FReferenceSkeleton& RefSkeleton = skelMesh->GetSkeleton()->GetRefSkeleton();
        const FTransform BoneTransform = RefSkeleton.GetRawRefBonePose()[BoneIndex];
        const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);

        // 1. AABB 계산용 기본 정보
        FVector BoxCenter(0, 0, 0), BoxExtent(1,1,1);
        FTransform ElementTransform = BoneTransform;

#if Cal_AABB_From_BonePos
        for (int32 ChildIndex = 0; ChildIndex < RefSkeleton.GetRawBoneNum(); ++ChildIndex)
        {
            if (RefSkeleton.GetParentIndex(ChildIndex) == BoneIndex)
            {
                const FTransform& ChildTransform = RefSkeleton.GetRawRefBonePose()[ChildIndex];
                FVector LocalDelta = ChildTransform.GetTranslation();
                BoxExtent = FVector::GetAbs(LocalDelta) * 0.5f; // 길이의 절반
                BoxCenter = LocalDelta * 0.5f;
                break;
            }
        }

        if (BoxExtent.IsNearlyZero())
        {
            BoxExtent = FVector(1, 1, 1);
            BoxCenter = FVector::ZeroVector;
        }
        ElementTransform.AddToTranslation(BoxCenter);

#endif


        if (bs->GeomType == EFG_Box)
        {
            FKBoxElem BoxElem;
            BoxElem.SetTransform(ElementTransform);
            BoxElem.Center = BoxCenter;

            BoxElem.X = BoxExtent.X * 2.0f * 1.01f; // Graphics Glitch 방지용으로 1% 추가
            BoxElem.Y = BoxExtent.Y * 2.0f * 1.01f;
            BoxElem.Z = BoxExtent.Z * 2.0f * 1.01f;

            bs->AggGeom.BoxElems.Add(BoxElem);
        }
        else if (bs->GeomType == EFG_Sphere)
        {
            FKSphereElem SphereElem;

            SphereElem.Center = ElementTransform.GetTranslation();
            SphereElem.Radius = BoxExtent.GetMax() * 1.01f;

            bs->AggGeom.SphereElems.Add(SphereElem);
        }
        else if (bs->GeomType == EFG_Sphyl)
        {
            FKSphylElem SphylElem;

            /* Z축 정렬 : 가장 긴 길이 축이 Z가 되도록 수동을 회전 -> 본의 길이 방향과 일치하도록
             * ex) X, Y 축이 가장 축이 길다면 해당 축을 Z축으로 회전
             */
            if (BoxExtent.X > BoxExtent.Z && BoxExtent.X > BoxExtent.Y)
            {
                //X축이 가장 길다면 회전: X-axis into Z-axis
                SphylElem.SetTransform(FTransform(FQuat(FVector(0, 1, 0), -PI * 0.5f)) * ElementTransform);
                SphylElem.Radius = FMath::Max(BoxExtent.Y, BoxExtent.Z) * 1.01f;
                SphylElem.Length = BoxExtent.X * 1.01f;

            }
            else if (BoxExtent.Y > BoxExtent.Z && BoxExtent.Y > BoxExtent.X)
            {
                //Y축이 가장 길다면 회전: Y-axis into Z-axis
                SphylElem.SetTransform(FTransform(FQuat(FVector(1, 0, 0), PI * 0.5f)) * ElementTransform);
                SphylElem.Radius = FMath::Max(BoxExtent.X, BoxExtent.Z) * 1.01f;
                SphylElem.Length = BoxExtent.Y * 1.01f;
            }

            else
            {
                // Z축이 가장 길다면 그대로 사용
                SphylElem.SetTransform(ElementTransform);

                SphylElem.Radius = FMath::Max(BoxExtent.X, BoxExtent.Y) * 1.01f;
                SphylElem.Length = BoxExtent.Z * 1.01f;
            }

            bs->AggGeom.SphylElems.Add(SphylElem);
        }
        else if (bs->GeomType == EFG_SingleConvexHull || bs->GeomType == EFG_MultiConvexHull)
        {
            // TArray<FVector> Verts;
            // TArray<uint32> Indices;
        }

        return true;
    }
}
