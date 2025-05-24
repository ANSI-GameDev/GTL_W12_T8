#pragma once
#include "Delegates/DelegateCombination.h"
#include "Engine/EngineTypes.h"
#include "Math/Vector.h"
#include "UObject/NameTypes.h"

namespace physx
{
class PxD6Joint;
}

struct FConstraintInstanceBase
{
    FConstraintInstanceBase();
    void Reset();

    int32 ConstraintIndex;
};

struct FConstraintInstance : public FConstraintInstanceBase
{
public:
    FConstraintInstance();

    physx::PxD6Joint* ConstraintData;
    
    FName JointName;

    
    /** 
     *	Name of first bone (body) that this constraint is connecting. 
     *	This will be the 'child' bone in a PhysicsAsset.
     */
    FName ChildConstraintBone;
    FVector ChildPos;
    FVector ChildPriAxis;
    FVector ChildSecAxis;

    /** 
     *	Name of second bone (body) that this constraint is connecting. 
     *	This will be the 'parent' bone in a PhysicsAset.
     */
    FName ParentConstraintBone;
    FVector ParentPos;
    FVector ParentPriAxis;
    FVector ParentSecAxis;

    float AverageMass;
    FRotator AngularRotationOffset;
    
    /** Get the child bone name */
    const FName& GetChildBoneName() const { return ChildConstraintBone; }
    
    /** Get the parent bone name */
    const FName& GetParentBoneName() const { return ParentConstraintBone; }
};
