#pragma once
#include "AggregateGeom.h"
#include "Math/Matrix.h"
#include "Math/Transform.h"
#include "Math/Vector.h"
#include "PhysicsCore/Public/BodyInstanceCore.h"
#include "UObject/NameTypes.h"

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
        /*Inherits the degrees of freedom from the project settings.*/
        Default,
        /*Specifies which axis to freeze rotation and movement along.*/
        SixDOF,
        /*Allows 2D movement along the Y-Z plane.*/
        YZPlane,
        /*Allows 2D movement along the X-Z plane.*/
        XZPlane,
        /*Allows 2D movement along the X-Y plane.*/
        XYPlane,
        /*Allows 2D movement along the plane of a given normal*/
        CustomPlane,
        /*No constraints.*/
        None
    };
}

struct FBodyInstance : public FBodyInstanceCore
{
public:
    /** 
     *	Index of this BodyInstance within the SkeletalMeshComponent/PhysicsAsset. 
     *	Is INDEX_NONE if a single body component
     */
    int32 InstanceBodyIndex;

    /** When we are a body within a SkeletalMeshComponent, we cache the index of the bone we represent, to speed up sync'ing physics to anim. */
    int16 InstanceBoneIndex;
    
    /** Current scale of physics - used to know when and how physics must be rescaled to match current transform of OwnerComponent. */
    FVector Scale3D;

    physx::PxRigidDynamic* RigidBody = nullptr;
    FPhysScene* MyScene = nullptr;
    
    FTransform WorldTransform;
    FVector LinearVelocity;
    FVector AngularVelocity;
    
    float Mass = 1.0f;
    float InverseMass = 1.0f;

    bool bSimulatePhysics = true;
    bool bEnableGravity = true;
    bool bIsKinematic = false;
    
    /** When per-shape collision responses are changed at runtime, state is stored in an optional array of per-shape
    *	collision response settings. If bShapeCollisionResponsesIsSet is false, the base body instance's CollisionResponses member is used for all shapes. */
    // TArray<TPair<int32, FCollisionResponse>> ShapeCollisionResponses;

    /** [Physx Only] Locks physical movement along specified axis.*/
    EDOFMode::Type DOFMode;
    /** Collision Profile Name **/
    FName CollisionProfileName;

public:
    // void ApplyForce(FVector Force);
    // void ApplyTorque(FVector Torque);
    // void AddImpulse(FVector Impulse);

    void SetTransformRigidBody(FTransform MoveLocation);
    
    //해당 BodyInstance를 PhysScene에 등록시켜주는 작업
    void InitBody(UBodySetup* InBodySetup, const FVector& InBodyWorldPosition, FPhysScene* InScene);
    void AttachShapes(const FKAggregateGeom& InAggregateGeom, FPhysScene* InScene);

    void SetWorldTransform(const FTransform& T) { WorldTransform = T; }
    FTransform GetWorldTransform() const { return WorldTransform; }

    // void SetWorldMatrix(const FMatrix& InMatrix){WorldMatrix = InMatrix;}
    // FMatrix GetWorldMatrix(){return WorldMatrix;}

    void SetLinearVelocity(FVector V) {LinearVelocity = V;}
    void SetAngularVelocity(FVector AV) {AngularVelocity = AV;}

    void SetbSimulatePhysics(bool b){ bSimulatePhysics = b; }
    void SetbEnableGravity(bool b){ bEnableGravity = b; }
    
    void UpdatePhysics();
    FTransform ConvertPxTransformToFTransform(const physx::PxTransform& InTransform);
    physx::PxTransform ConvertFTransformToPxTransform(const FTransform& InTransform);
    void ConvertPxMatToFMat(FMatrix& OutFMatrix, physx::PxMat44 InMat);
    physx::PxVec3 ConvertFVecToPxVec(const FVector& InVec);
};
