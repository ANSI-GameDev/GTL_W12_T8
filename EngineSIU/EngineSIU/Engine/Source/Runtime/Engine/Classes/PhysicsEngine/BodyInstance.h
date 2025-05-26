#pragma once
#include "Engine/EngineTypes.h"
#include "Math/Vector.h"
#include "PhysicsCore/BodyInstanceCore.h"
#include "UObject/NameTypes.h"

namespace EDOFMode
{
    enum Type : int
    {
        Default,
        SixDOF,
        YZPlane,
        XZPlane,
        XYPlane,
        CustomPlane,
        None
    };
}

struct FConstraintInstance;
struct FBodyInstance : public FBodyInstanceCore
{
    /* SkeletalMeshComponent / PhysicAsset 내부의 BodyInstance 인덱스
     * 단일 Body 컴포넌트인 경우 INDEX_NONE입니다.
     */
    int32 InstanceBodyIndex;
    int16 InstanceBoneIndex;

    ECollisionEnabled::Type CollisionEnabled;
    EDOFMode::Type DOFMode;

    FName CollisionProfileName;
    FVector Scale3D;

    float LinearDamping;
    float AngularDamping;
    FVector COMNudge;
    float MassScale;

    /** [Physx Only] Constraint used to allow for easy DOF setup per bodyinstance */
    FConstraintInstance* DOFConstraint;

    uint8 bUseCCD : 1;

    uint8 bLockTranslation : 1;
    uint8 bLockRotation : 1;
    uint8 bLockXTranslation : 1;
    uint8 bLockYTranslation : 1;
    uint8 bLockZTranslation : 1;

    uint8 bLockXRotation : 1;
    uint8 bLockYRotation : 1;
    uint8 bLockZRotation : 1;

    uint8 bOverrideMaxAngularVelocity : 1;
};
