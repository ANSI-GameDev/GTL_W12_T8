#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

/* 충돌 형상 정의하는 최소한의 데이터 정의
 *
 */
class UBodySetupCore : public UObject
{
    DECLARE_CLASS(UBodySetupCore, UObject)
public:
    UBodySetupCore() = default;

    FName BoneName;


};
