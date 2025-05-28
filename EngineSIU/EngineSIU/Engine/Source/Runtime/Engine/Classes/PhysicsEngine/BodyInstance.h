#pragma once
#include "Engine/EngineTypes.h"
#include "AggregateGeom.h"
#include "PhysicsCore/BodyInstanceCore.h"

class FPhysScene;
class UBodySetup;

namespace physx
{
class PxShape;
class PxRigidDynamic;
class PxVec3;
class PxMat44;
class PxTransform;
class PxRigidActor;
}

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
    
    /** Current scale of physics - used to know when and how physics must be rescaled to match current transform of OwnerComponent. */
    FVector Scale3D;

    physx::PxRigidDynamic* RigidBody = nullptr;
    FPhysScene* MyScene = nullptr;
    
    FTransform WorldTransform;

    FVector LinearVelocity;
    FVector AngularVelocity;
    FVector COMNudge;

    bool bSimulatePhysics = true;
    bool bEnableGravity = true;
    bool bIsKinematic = false;
    
    float LinearDamping;
    float AngularDamping;

    float MassScale;
    FVector InertiaTensorScale;

    /** [Physx Only] Locks physical movement along specified axis.*/
    EDOFMode::Type DOFMode;
    /** [Physx Only] Constraint used to allow for easy DOF setup per bodyinstance */
    FConstraintInstance* DOFConstraint;
public:
    // void ApplyForce(FVector Force);
    // void ApplyTorque(FVector Torque);
    // void AddImpulse(FVector Impulse);

    void SetTransformRigidBody(FTransform MoveLocation);
    
    //해당 BodyInstance를 PhysScene에 등록시켜주는 작업
    void InitBody(UBodySetup* InBodySetup, const FTransform& InBodyWorldTransform, FPhysScene* InScene);
    void AttachShapes(const FKAggregateGeom& InAggregateGeom, FPhysScene* InScene);

    void SetWorldTransform(const FTransform& T) { WorldTransform = T; }
    FTransform GetWorldTransform() const { return WorldTransform; }
    UBodySetup* GetBodySetup() const;
    physx::PxRigidDynamic* GetPxRigidBoDynamic() const;

    uint8 bUseCCD : 1;

    uint8 bLockTranslation : 1;
    uint8 bLockXTranslation : 1;
    uint8 bLockYTranslation : 1;
    uint8 bLockZTranslation : 1;

    uint8 bLockRotation : 1;
    uint8 bLockXRotation : 1;
    uint8 bLockYRotation : 1;
    uint8 bLockZRotation : 1;

    uint8 bOverrideMaxAngularVelocity : 1;
 
    void SetLinearVelocity(FVector V) {LinearVelocity = V;}
    void SetAngularVelocity(FVector AV) {AngularVelocity = AV;}

    void SetbSimulatePhysics(bool b){ bSimulatePhysics = b; }
    void SetbEnableGravity(bool b){ bEnableGravity = b; }
    
    void UpdatePhysics();
};
