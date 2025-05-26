#include "PhysicsAsset.h"

#include "BodySetup.h"
#include "PhysicsConstraintTemplate.h"

void UPhysicsAsset::UpdateBodySetupIndexMap()
{
    // update BodySetupIndexMap
    BodySetupIndexMap.Empty();

    for (int32 i = 0; i < BodySetup.Num(); i++)
    {
        if (BodySetup[i])
        {
            BodySetupIndexMap.Add(BodySetup[i]->BoneName, i);
        }
    }
}

void UPhysicsAsset::UpdateBoundsBodiesArray()
{
    // Update the bounds bodies array based on the current body setups
    BoundsBodies.Empty();

    for (int32 i = 0; i < BodySetup.Num(); i++)
    {
        if (BodySetup[i]&& BodySetup[i]->bConsiderForBounds)
        {
            BoundsBodies.Add(i);
        }
    }
}


const TArray<FName>& UPhysicsAsset::GetConstraintProfileNames() const
{
    return ConstraintProfiles;
}

int32 UPhysicsAsset::FindBodyIndex(FName bodyName) const
{
    const int32* IdxData = BodySetupIndexMap.Find(bodyName);
    if (IdxData)
    {
        return *IdxData;
    }

    return INDEX_NONE;
}

int32 UPhysicsAsset::FindConstraintIndex(FName ConstraintName) const
{
    for (int32 i = 0; i < ConstraintSetup.Num(); ++i)
    {
        if (ConstraintSetup[i]->DefaultInstance.JointName == ConstraintName)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

int32 UPhysicsAsset::FindConstraintIndex(FName Bone1Name, FName Bone2Name) const
{
    for (int32 i = 0; i < ConstraintSetup.Num(); ++i)
    {
        const FConstraintInstance& Instance = ConstraintSetup[i]->DefaultInstance;
        if (Instance.ConstraintBone1 == Bone1Name && Instance.ConstraintBone2 == Bone2Name)
        {
            return i;
        }
    }
    return INDEX_NONE;
}


