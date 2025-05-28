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
                const FName ConstraintName = FName(*FString::Printf(TEXT("%s->%s_Constraint"), *ChildName.ToString(), *ParentName.ToString()));

                UPhysicsConstraintTemplate* CS = nullptr;
                int32 NewConstraintIndex = CreateNewConstraint(PhysicsAsset, ConstraintName, CS, ParentName, ChildName);
                CS = PhysicsAsset->ConstraintSetup[NewConstraintIndex];


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

        /* Body Setup의 BoneIndex , */
        int32 BodySetupIndex = PhysAsset->BodySetup.Add(NewBodySetup);
        NewBodySetup->BoneName = InBoneName;
        NewBodySetup->BoneIndex = PhysAsset->GetPreviewMesh()->GetSkeleton()->GetRefSkeleton().FindRawBoneIndex(InBoneName);
        NewBodySetup->ParentBoneIndex = PhysAsset->GetPreviewMesh()->GetSkeleton()->GetRefSkeleton().GetParentIndex(NewBodySetup->BoneIndex);

        PhysAsset->UpdateBodySetupIndexMap();
        PhysAsset->UpdateBoundsBodiesArray();

        return BodySetupIndex;
    }

    bool CreateCollisionFromBoneInternal(UBodySetup* bs, USkeletalMesh* skelMesh, int32 BoneIndex)
    {
        if (!bs || !skelMesh || !skelMesh->GetSkeleton()->GetRefSkeleton().IsValidRawIndex(BoneIndex))
        {
            UE_LOG(ELogLevel::Error, TEXT("Invalid parameters"));
            return false;
        }
        //if (FConstraintInstance::IsEndEffectorJoint(bs->BoneName)) return true;

        const FReferenceSkeleton& RefSkeleton = skelMesh->GetSkeleton()->GetRefSkeleton();
        // 원본은 RawRefBonePose 썼지만, 스켈레탈 메시 전체 참조 좌표계는 GetComposedRefPoseMatrix 로 꺼내도 됩니다.
        FTransform ElementTransform = RefSkeleton.GetRawRefBonePose()[BoneIndex];

        // fallback
        FVector BoxExtent(1.f);
        FVector BoxCenter = FVector::ZeroVector;

        // 부모↔자식 벡터 계산
        int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
        FTransform ParentWorld;
        FTransform ThisWorld = RefSkeleton.GetRawRefBonePose()[BoneIndex];
        if (ParentIndex != INDEX_NONE)
        {
            ParentWorld = RefSkeleton.GetRawRefBonePose()[ParentIndex];
            // (1) 월드 좌표계 위치 얻기
            for (int32 P = RefSkeleton.GetParentIndex(ParentIndex); P != INDEX_NONE; P = RefSkeleton.GetParentIndex(P))
                ParentWorld = RefSkeleton.GetRawRefBonePose()[P] * ParentWorld;

            for (int32 T = RefSkeleton.GetParentIndex(BoneIndex); T != INDEX_NONE; T = RefSkeleton.GetParentIndex(T))
                ThisWorld = RefSkeleton.GetRawRefBonePose()[T] * ThisWorld;

            FVector ParentPos = ParentWorld.GetLocation();
            FVector ThisPos = ThisWorld.GetLocation();
            FVector Dir = (ThisPos - ParentPos);
            float Length = Dir.Size();
            Dir = Dir.GetSafeNormal();
            if (Length < KINDA_SMALL_NUMBER)
            {
                Dir = FVector(0, 0, 1);
                Length = 5.f;
            }


            // Z축에만 half-length를 실어줌
            BoxCenter = (ParentPos + ThisPos) * 0.5f;
            BoxExtent = FVector(1.f, 1.f, Length * 0.5f);

            /* Y+를 Z+로 보정하고 (PhysX Y축 캡슐을 Z축 기준으로 회전) //[미사용] Z축 기준으로 본 방향을 향하도록 회전시킴*/
            FQuat CapsuleDirRotation = FQuat::FindBetweenNormals(FVector(1, 0, 0), Dir);
            //FQuat PhysX_YtoZ_Rotation = FQuat(FVector(1, 0, 0), PI / 2); // 90도 회전 (X축 기준)
            //FQuat FinalRotation = CapsuleDirRotation/**PhysX_YtoZ_Rotation*/;

            ElementTransform = FTransform(CapsuleDirRotation, BoxCenter);
        }
        else // 자신이 root bone일 때
        {
            ElementTransform = FTransform(FQuat::Identity, ThisWorld.GetLocation());
            BoxExtent = FVector::ZeroVector;
        }

        // --- 이제 GeomType 별로 추가 ---
        if (bs->GeomType == EFG_Sphyl)
        {
            FKSphylElem SphylElem;

            // ★ 반지름: X,Y 중 큰 값 / 절반높이: Z축 값
            float CapsuleRadius = FMath::Max(BoxExtent.X, BoxExtent.Y) * 1.01f;
            CapsuleRadius = FMath::Max(CapsuleRadius, 1.f);
            float CapsuleHalfLength = BoxExtent.Z;
            CapsuleHalfLength = FMath::Max(CapsuleHalfLength - CapsuleRadius, 0.7f);
            //CapsuleHalfLength -= CapsuleRadius;

            SphylElem.Center = ThisWorld.GetLocation() - ElementTransform.GetLocation();
            SphylElem.RQuat = ElementTransform.GetRotation();
            SphylElem.Radius = CapsuleRadius;
            SphylElem.Length = CapsuleHalfLength * 2.f;  // PhysX는 전체 길이

            bs->AggGeom.SphylElems.Add(SphylElem);
        }
        else if (bs->GeomType == EFG_Box)
        {
            FKBoxElem BoxElem;
            BoxElem.SetTransform(ElementTransform);
            BoxElem.Center = FVector::ZeroVector;
            BoxElem.Extent = BoxExtent * 2.f * 1.01f;
            bs->AggGeom.BoxElems.Add(BoxElem);
        }
        else if (bs->GeomType == EFG_Sphere)
        {
            FKSphereElem SphereElem;
            SphereElem.Center = ElementTransform.GetTranslation();
            SphereElem.Radius = BoxExtent.GetMax() * 1.01f;
            bs->AggGeom.SphereElems.Add(SphereElem);
        }

        return true;
    }
}
