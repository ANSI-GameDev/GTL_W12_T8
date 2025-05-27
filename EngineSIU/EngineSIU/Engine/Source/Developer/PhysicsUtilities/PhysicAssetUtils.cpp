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
            UE_LOG(ELogLevel::Error, TEXT("Invalid parameters"));
            return false;
        }

        const FReferenceSkeleton& RefSkeleton = skelMesh->GetSkeleton()->GetRefSkeleton();
        // 원본은 RawRefBonePose 썼지만, 스켈레탈 메시 전체 참조 좌표계는 GetComposedRefPoseMatrix 로 꺼내도 됩니다.
        FTransform ElementTransform = RefSkeleton.GetRawRefBonePose()[BoneIndex];

        // fallback
        FVector BoxExtent(1.f);
        FVector BoxCenter = FVector::ZeroVector;

        // 부모↔자식 벡터 계산
        int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
        if (ParentIndex != INDEX_NONE)
        {
            // (1) 월드 좌표계 위치 얻기
            FTransform ParentWorld = RefSkeleton.GetRawRefBonePose()[ParentIndex];
            for (int32 P = RefSkeleton.GetParentIndex(ParentIndex); P != INDEX_NONE; P = RefSkeleton.GetParentIndex(P))
                ParentWorld = RefSkeleton.GetRawRefBonePose()[P] * ParentWorld;

            FTransform ThisWorld = RefSkeleton.GetRawRefBonePose()[BoneIndex];
            for (int32 T = RefSkeleton.GetParentIndex(BoneIndex); T != INDEX_NONE; T = RefSkeleton.GetParentIndex(T))
                ThisWorld = RefSkeleton.GetRawRefBonePose()[T] * ThisWorld;

            FVector ParentPos = ParentWorld.GetLocation();
            FVector ThisPos = ThisWorld.GetLocation();
            FVector Dir = (ThisPos - ParentPos);
            float Length = Dir.Size();
            Dir = Dir.GetSafeNormal();
            //if (Length < KINDA_SMALL_NUMBER)
            //{
            //    Dir = FVector(0, 0, 1);
            //    Length = 10.f;
            //}
            //else
            //{
            //    Dir /= Length;
            //}

            // Z축에만 half-length를 실어줌
            BoxCenter = (ParentPos + ThisPos) * 0.5f;
            BoxExtent = FVector(1.f, 1.f, Length * 0.5f);

            FQuat CapsuleDirRotation = FQuat::FindBetweenNormals(FVector(1, 0, 0), Dir);
            FQuat PhysX_YtoZ_Rotation = FQuat(FVector(1, 0, 0), PI / 2); // 90도 회전 (X축 기준)
            FQuat FinalRotation = CapsuleDirRotation*PhysX_YtoZ_Rotation;

            ElementTransform = FTransform(FinalRotation, BoxCenter);
        }
        else
        {
            
        }

        // --- 이제 GeomType 별로 추가 ---
        if (bs->GeomType == EFG_Sphyl)
        {
            FKSphylElem SphylElem;

            // ★ 반지름: X,Y 중 큰 값 / 절반높이: Z축 값
            float CapsuleRadius = FMath::Max(BoxExtent.X, BoxExtent.Y) * 1.01f;
            CapsuleRadius = FMath::Max(CapsuleRadius, 1.f);
            float CapsuleHalfLength = BoxExtent.Z;
            CapsuleHalfLength = FMath::Max(CapsuleHalfLength, 1.f);

            SphylElem.Center = ElementTransform.GetLocation();
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


//    bool CreateCollisionFromBoneInternal(UBodySetup* bs, USkeletalMesh* skelMesh, int32 BoneIndex)
//    {
//        if (!bs || !skelMesh || !skelMesh->GetSkeleton()->GetRefSkeleton().IsValidRawIndex(BoneIndex))
//        {
//            UE_LOG(ELogLevel::Error, TEXT("FPhysicsAssetUtils::CreateCollisionFromBoneInternal : Invalid parameters"));
//            return false;
//        }
//
//        const FReferenceSkeleton& RefSkeleton = skelMesh->GetSkeleton()->GetRefSkeleton();
//        const FTransform BoneTransform = RefSkeleton.GetRawRefBonePose()[BoneIndex];
//        const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
//
//        // 1. AABB 계산용 기본 정보
//        FVector BoxCenter(0, 0, 0), BoxExtent(1,1,1);
//        FTransform ElementTransform = BoneTransform;
//
//#if Cal_AABB_From_BonePos
//        for (int32 ChildIndex = 0; ChildIndex < RefSkeleton.GetRawBoneNum(); ++ChildIndex)
//        {
//            if (RefSkeleton.GetParentIndex(ChildIndex) == BoneIndex)
//            {
//                const FTransform& ChildTransform = RefSkeleton.GetRawRefBonePose()[ChildIndex];
//                FVector LocalDelta = ChildTransform.GetTranslation();
//
//                // TODO : 방어코드 수정 필요 = 너무 작으면 기본값 적용
//                if (LocalDelta.SizeSquared() < KINDA_SMALL_NUMBER)
//                {
//                    LocalDelta = FVector(5.f, 5.f, 5.f); // 기본 방향 벡터
//                }
//
//                BoxExtent = FVector::GetAbs(LocalDelta) * 0.5f; // 길이의 절반
//                BoxCenter = LocalDelta * 0.5f;
//                break;
//            }
//        }
//
//        if (BoxExtent.IsNearlyZero())
//        {
//            BoxExtent = FVector(5);
//            BoxCenter = FVector::ZeroVector;
//        }
//        ElementTransform.AddToTranslation(BoxCenter);
//
//#endif
//
//
//        if (bs->GeomType == EFG_Box)
//        {
//            FKBoxElem BoxElem;
//            BoxElem.SetTransform(ElementTransform);
//            BoxElem.Center = BoxCenter;
//
//            BoxElem.Extent = FVector(BoxExtent.X * 2.0f * 1.01f,
//            BoxExtent.Y * 2.0f * 1.01f,
//            BoxExtent.Z * 2.0f * 1.01f); // Graphics Glitch 방지용으로 1% 추가
//
//            bs->AggGeom.BoxElems.Add(BoxElem);
//        }
//        else if (bs->GeomType == EFG_Sphere)
//        {
//            FKSphereElem SphereElem;
//
//            SphereElem.Center = ElementTransform.GetTranslation();
//            SphereElem.Radius = BoxExtent.GetMax() * 1.01f;
//
//            bs->AggGeom.SphereElems.Add(SphereElem);
//        }
//        //else if (bs->GeomType == EFG_Sphyl)
//        //{
//        //    FKSphylElem SphylElem;
//
//
//        //    /* Z축 정렬 : 가장 긴 길이 축이 Z가 되도록 수동을 회전 -> 본의 길이 방향과 일치하도록
//        //     * ex) X, Y 축이 가장 축이 길다면 해당 축을 Z축으로 회전
//        //     */
//        //    if (BoxExtent.X > BoxExtent.Z && BoxExtent.X > BoxExtent.Y)
//        //    {
//        //        //X축이 가장 길다면 회전: X-axis into Z-axis
//        //        SphylElem.SetTransform(FTransform(FQuat(FVector(0, 1, 0), -PI * 0.5f)) * ElementTransform);
//        //        SphylElem.Radius = FMath::Max(BoxExtent.Y, BoxExtent.Z) * 1.01f;
//        //        SphylElem.Length = BoxExtent.X * 1.01f;
//
//        //    }
//        //    else if (BoxExtent.Y > BoxExtent.Z && BoxExtent.Y > BoxExtent.X)
//        //    {
//        //        //Y축이 가장 길다면 회전: Y-axis into Z-axis
//        //        SphylElem.SetTransform(FTransform(FQuat(FVector(1, 0, 0), PI * 0.5f)) * ElementTransform);
//        //        SphylElem.Radius = FMath::Max(BoxExtent.X, BoxExtent.Z) * 1.01f;
//        //        SphylElem.Length = BoxExtent.Y * 1.01f;
//        //    }
//
//        //    else
//        //    {
//        //        // Z축이 가장 길다면 그대로 사용
//        //        SphylElem.SetTransform(ElementTransform);
//
//        //        SphylElem.Radius = FMath::Max(BoxExtent.X, BoxExtent.Y) * 1.01f;
//        //        SphylElem.Length = BoxExtent.Z * 1.01f;
//        //    }
//
//        //    float SafeRadius = FMath::Max(SphylElem.Radius, 1.f);
//        //    float SafeLength = FMath::Max(SphylElem.Length, 1.f);
//
//        //    SphylElem.Radius = SafeRadius;
//        //    SphylElem.Length = SafeLength;
//
//        //    bs->AggGeom.SphylElems.Add(SphylElem);
//        //}
//        else if (bs->GeomType == EFG_Sphyl)
//        {
//            FKSphylElem SphylElem;
//
//            // 1) 컴포넌트 공간에서 자식·부모 본 위치
//            const FTransform BoneCS = RefSkeleton.GetRawRefBonePose()[BoneIndex];
//            const FVector  ChildCSPos = BoneCS.GetLocation();
//
//            int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
//            FVector ParentCSPos = (ParentIndex != INDEX_NONE)
//                ? RefSkeleton.GetRawRefBonePose()[ParentIndex].GetLocation()
//                : (ChildCSPos + FVector(0, 0, 10.f));
//
//            // 2) 본 로컬 공간으로 변환
//            FVector ChildLocalPos = BoneTransform.InverseTransformPosition(ChildCSPos);
//            FVector ParentLocalPos = BoneTransform.InverseTransformPosition(ParentCSPos);
//
//            // 3) 델타·중심·방향·길이 계산
//            FVector DeltaLocal = ParentLocalPos - ChildLocalPos;
//            float   Len = DeltaLocal.Size();
//            FVector DirLocal = (Len > KINDA_SMALL_NUMBER)
//                ? (DeltaLocal / Len)
//                : FVector(0, 0, 1);
//
//            FVector CenterLocal = (ChildLocalPos + ParentLocalPos) * 0.5f;
//
//            // 4) OBB 방향(박스용 회전) 가져오기
//            const FQuat OBBoRot = ElementTransform.GetRotation();
//
//            // 5) 캡슐 축(Z) → 자식-부모 방향 정렬
//            const FQuat AlignQuat = FQuat::FindBetweenNormals(FVector(0, 0, 1), DirLocal);
//            const FQuat CapsuleLocalRot = (OBBoRot * AlignQuat).GetNormalized();
//
//            // 6) 로컬 트랜스폼 조합 & 컴포넌트 공간으로 변환
//            //    (BoneTransform: 본→컴포넌트)
//            FTransform CapsuleLocalTM(CapsuleLocalRot, CenterLocal);
//            SphylElem.SetTransform(CapsuleLocalTM * BoneTransform);
//
//            // 7) 반지름·길이 설정
//            float CapsuleRadius = FMath::Max(BoxExtent.X, BoxExtent.Y) * 1.01f;
//            CapsuleRadius = FMath::Max(CapsuleRadius, 1.0f);
//
//            float CapsuleFullLen = FMath::Max(Len * 1.01f, CapsuleRadius * 2.0f);
//
//            SphylElem.Radius = CapsuleRadius;
//            SphylElem.Length = CapsuleFullLen;
//
//            bs->AggGeom.SphylElems.Add(SphylElem);
//        }
//        
//        else if (bs->GeomType == EFG_SingleConvexHull || bs->GeomType == EFG_MultiConvexHull)
//        {
//            // TArray<FVector> Verts;
//            // TArray<uint32> Indices;
//        }
//
//        return true;
//    }





}
