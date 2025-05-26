#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

enum PhysicsType : int
{
    /** Follow owner. */
    PhysType_Default,
    /** Do not follow owner, but make kinematic. */
    PhysType_Kinematic,		
    /** Do not follow owner, but simulate. */
    PhysType_Simulated,
};


/* 충돌 형상 정의하는 최소한의 데이터 정의
 *
 */
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
