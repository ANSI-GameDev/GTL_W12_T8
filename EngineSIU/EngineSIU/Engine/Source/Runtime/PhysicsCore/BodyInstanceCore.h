#pragma once
#include "HAL/PlatformType.h"

class UBodySetupCore;

struct FBodyInstanceCore
{

    /* 본 인스턴스가 초기화된 출처 주소입니다 */
    UBodySetupCore* BodySetup;

    /* 시뮬레이션 하기 위해,
     * Skeletal Mesh Component : Physics asset setup과 SkeletalMesh Asset에 장착되어야 합니다
     * Static Mesh Component : StaticMesh asset에 간단한 Collision이 설정되어 있어야 합니다
     */
    uint8 bSimulatePhysics : 1;
    uint8 bOverrideMass : 1;
    uint8 bEnableGravity : 1;
};
