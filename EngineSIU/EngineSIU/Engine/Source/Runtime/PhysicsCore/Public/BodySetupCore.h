#pragma once
#include "BodySetupEnums.h"
#include "UObject/ObjectMacros.h"

class UBodySetupCore : public UObject
{
    DECLARE_CLASS(UBodySetupCore, UObject)
public:
    UBodySetupCore() = default;

    FName BoneName;

    uint32 BoneIndex;
    
    //PhysicsType Default, Kinematic, simulated
    PhysicsType PhysicsType;

    //Collision Type for this body
    uint8 CollisionResponse : 1 = true;
};
