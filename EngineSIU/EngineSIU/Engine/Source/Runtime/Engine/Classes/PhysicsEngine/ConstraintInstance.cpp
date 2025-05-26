#include "ConstraintInstance.h"
#include "PhysicsCore/Chaos/ChaosEngineInterface.h"
#include "PhysicsAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/EnumClassFlags.h"
#include "UserInterface/Console.h"

FConstraintProfileProperties::FConstraintProfileProperties()
    : bDisableCollision(true) // 관절 사이의 충돌 기본적으로 비활성화
{
}

FConstraintInstanceBase::FConstraintInstanceBase()
{
    Reset();
}

void FConstraintInstanceBase::Reset()
{
    ConstraintIndex = 0;
    PhysScene = nullptr;
}

FPhysScene* FConstraintInstanceBase::GetPhysScene() const
{
    return PhysScene;
}

const FPhysScene* FConstraintInstanceBase::GetPhysicsScene() const
{
    return PhysScene;
}

void FConstraintInstance::UpdateLinearLimit()
{
}

void FConstraintInstance::UpdateAngularLimit()
{
}

/*
 * 여기서 모든 PhysX 엔진의 Constraint 인스턴스 생성
 */
void FConstraintInstance::InitConstraint(FBodyInstance* Body1, FBodyInstance* Body2, float Scale, USkeletalMeshComponent* OwningComponent)
{

}

/* 두 Bone 간 RefPose 기준 상대 위치 계산 */
FTransform CalculateRelativeBoneTransform(const FName ToBoneName, const FName FromBoneName, const FReferenceSkeleton& RefSkeleton)
{
    FTransform RelativeBoneTransform = FTransform::Identity;

    const TArray<FTransform>& LocalPose = RefSkeleton.GetRawRefBonePose();
    int32 ToBoneAncestorIndex = RefSkeleton.FindBoneIndex(ToBoneName);
    int32 FromBoneAncestorIndex = RefSkeleton.FindBoneIndex(FromBoneName);

    if (!LocalPose.IsValidIndex(ToBoneAncestorIndex) || !LocalPose.IsValidIndex(FromBoneAncestorIndex))
    {
        UE_LOG(ELogLevel::Error, TEXT("CalculateRelativeBoneTransform : Invalid bone indices for %s or %s"), *ToBoneName.ToString(), *FromBoneName.ToString());
        return FTransform::Identity;
    }

    FTransform ToCommonBasis = FTransform::Identity;
    FTransform FromCommonBasis = FTransform::Identity;

    /* 자식부터 부모까지 모든 Bone을 순회하며 Transform 누적
     * 둘 다 동일한 조상 Bone을 가질 때까지
     */
    while (LocalPose.IsValidIndex(ToBoneAncestorIndex) && LocalPose.IsValidIndex(FromBoneAncestorIndex))
    {
        if (ToBoneAncestorIndex > FromBoneAncestorIndex)
        {
            ToCommonBasis = ToCommonBasis * LocalPose[ToBoneAncestorIndex];
            ToBoneAncestorIndex = RefSkeleton.GetParentIndex(ToBoneAncestorIndex);
        }
        else if (FromBoneAncestorIndex > ToBoneAncestorIndex)
        {
            FromCommonBasis = FromCommonBasis * LocalPose[FromBoneAncestorIndex];
            FromBoneAncestorIndex = RefSkeleton.GetParentIndex(FromBoneAncestorIndex);
        }
        else
        {
            break; // 공통 조상 발견
        }
    }

    if (ToBoneAncestorIndex == FromBoneAncestorIndex)
    {
        // 공통 조상 Bone을 찾았으므로, 두 Bone의 상대 Transform 계산
        RelativeBoneTransform = ToCommonBasis.GetRelativeTransform(FromCommonBasis);
    }
    else
    {
        UE_LOG(ELogLevel::Error, TEXT("CalculateRelativeBoneTransform : No common ancestor found for %s and %s"), *ToBoneName.ToString(), *FromBoneName.ToString());
    }

    return RelativeBoneTransform;
}

/*
 * 자식은 기본적으로 로컬 기준이므로 Identity 반환
 */
FTransform FConstraintInstance::CalculateDefaultChildTransform() const
{    
    return FTransform::Identity;
}

/* 	자식 기준 부모 Transform 계산 */
FTransform FConstraintInstance::CalculateDefaultParentTransform(const UPhysicsAsset* const PhysicsAsset) const
{
    if (PhysicsAsset)
    {
        if (USkeletalMesh* const PreviewSkelMesh = PhysicsAsset->GetPreviewMesh())
        {
            return CalculateRelativeBoneTransform(ConstraintBone1, ConstraintBone2, PreviewSkelMesh->GetSkeleton()->GetRefSkeleton());
        }
    }

    return FTransform::Identity;
}

/*
 * 두 Bone 간의 Constraint가 틀어지지 않도록
 * 자식과 부모 Bone의 위치 및 회전 좌표계를 맞추기 위해 축 / 위치 보정 수행
 */
void FConstraintInstance::SnapTransformsToDefault(const EConstraintTransformComponentFlags SnapFlags, const UPhysicsAsset* const PhysicsAsset)
{
    const FTransform ParentTransform = CalculateDefaultParentTransform(PhysicsAsset);
    const FTransform ChildTransform = CalculateDefaultChildTransform();

    if (EnumHasAnyFlags(SnapFlags, EConstraintTransformComponentFlags::ChildPosition))
    {
        SetRefPosition(EConstraintFrame::Frame1, ChildTransform.GetLocation());
    }

    if (EnumHasAnyFlags(SnapFlags, EConstraintTransformComponentFlags::ChildRotation))
    {
        SetRefOrientation(EConstraintFrame::Frame1, ChildTransform.GetUnitAxis(EAxis::X), ChildTransform.GetUnitAxis(EAxis::Y));
    }

    if (EnumHasAnyFlags(SnapFlags, EConstraintTransformComponentFlags::ParentPosition))
    {
        SetRefPosition(EConstraintFrame::Frame2, ParentTransform.GetLocation());
    }

    if (EnumHasAnyFlags(SnapFlags, EConstraintTransformComponentFlags::ParentRotation))
    {
        SetRefOrientation(EConstraintFrame::Frame2, ParentTransform.GetUnitAxis(EAxis::X), ParentTransform.GetUnitAxis(EAxis::Y));
    }
}

/** Constructor **/
FConstraintInstance::FConstraintInstance()
    : FConstraintInstanceBase()
    , LastKnownScale(1.f)
    , AngularRotationOffset(FRotator::ZeroRotator)
    , bScaleLinearLimits(true)
    , AverageMass(0.f)
{
    Pos1 = FVector(0.0f, 0.0f, 0.0f);
    PriAxis1 = FVector(1.0f, 0.0f, 0.0f);
    SecAxis1 = FVector(0.0f, 1.0f, 0.0f);

    Pos2 = FVector(0.0f, 0.0f, 0.0f);
    PriAxis2 = FVector(1.0f, 0.0f, 0.0f);
    SecAxis2 = FVector(0.0f, 1.0f, 0.0f);
}

void FConstraintInstance::CopyConstraintParamsFrom(const FConstraintInstance* FromInstance)
{
    if (FromInstance->PhysScene == nullptr)
    {
        UE_LOG(ELogLevel::Error, TEXT("FConstraintInst::CopyConstraintParamsFrom() : PhysScene is NULLPTR"));
    }
    *this = *FromInstance;
}

/* Constraint가 작동할 때 기준이 되는 두 개체의 위치 기준점(로컬 좌표계 상 위치) 지정 */
void FConstraintInstance::SetRefPosition(EConstraintFrame::Type Frame, const FVector& Position)
{
    /* Frame1 : 자식 본에 대한 위치 <-> Frame2 */
    if (Frame == EConstraintFrame::Frame1)
    {
        Pos1 = Position;
    }
    else
    {
        Pos2 = Position;
    }
}

/* Frame1 또는 Frame2의 참조 방향(Orientation) 설정
 * Primary 축과 Secondary 축을 지정하여 회전 방향을 정의
 * Primary 축은 Twist 방향, Secondary 축은 Swing 방향으로 사용됨
 */
void FConstraintInstance::SetRefOrientation(EConstraintFrame::Type Frame, const FVector& InPriAxis, const FVector& InSecAxis)
{
    // 정규화된 축을 사용
    FVector PriAxis = InPriAxis.GetSafeNormal();
    FVector SecAxis = InSecAxis.GetSafeNormal();
    FVector Binormal = FVector::CrossProduct(PriAxis, SecAxis).GetSafeNormal(); // Z축 = Pri × Sec

    // SecAxis를 PriAxis와 직교하도록 보정
    SecAxis = FVector::CrossProduct(Binormal, PriAxis).GetSafeNormal();

    if (Frame == EConstraintFrame::Frame1)
    {
        PriAxis1 = PriAxis;
        SecAxis1 = SecAxis;
    }
    else
    {
        PriAxis2 = PriAxis;
        SecAxis2 = SecAxis;
    }
}

const FName& FConstraintInstance::GetChildBoneName() const
{
    return ConstraintBone1; 
}

const FName& FConstraintInstance::GetParentBoneName() const
{
    return ConstraintBone2;
}

float FConstraintInstance::GetLinearLimit() const
{
    return ProfileInstance.LinearLimit.Limit;
}




