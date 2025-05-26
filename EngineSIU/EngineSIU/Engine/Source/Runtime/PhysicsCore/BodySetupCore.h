#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

enum PhysicsType : int
{
    /** Transform을 따라가며 시뮬레이션 없음 (Static처럼 작동) */
    PhysType_Default,
    /** 위치는 외부 제어 (애니메이션 등), 물리 연산은 안함 */
    PhysType_Kinematic,		
    /** 물리엔진 완전 위임, 중력/힘/충돌 적용됨 */
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
