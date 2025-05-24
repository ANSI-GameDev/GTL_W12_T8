#pragma once
#include "HAL/PlatformType.h"

class UBodySetupCore;

struct FBodyInstanceCore
{
public:
    //collision과 뼈에 관련된 정보가 담겨있음
    UBodySetupCore* BodySetUpCore;

    uint8 bSimulatePhysics : 1 = true;

    uint8 bEnableGravity : 1 = true;

    bool ShouldInstanceSimulatingPhysics() const;
};
