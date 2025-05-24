#pragma once

#include "UObject/ObjectMacros.h"

class USkeletalMesh;
class UBodySetup;

class UPhysicsAsset : public UObject
{
    DECLARE_CLASS(UPhysicsAsset, UObject)

    UPhysicsAsset() = default;
    
    TArray<UBodySetup*> BodySetup;

    USkeletalMesh* PreviewSkeletalMesh;
    
};
