#pragma once
#include "HAL/PlatformType.h"
class UPhysicsConstraintTemplate;
class USkeletalMesh;
class FName;
class UPhysicsAsset;

namespace FPhysicsAssetUtils
{
    /**
     * 주어진 USkeletalMesh를 사용하여 새로운 PhysicsAsset을 자동으로 생성합니다.
     * 각 본에 가중치가 부여된 버텍스를 이용해 대략적인 충돌 기하구조를 계산합니다.
     * 기본적으로 모든 관절에 대해 볼 앤 소켓(ball-and-socket) 조인트가 생성됩니다.
     *
     * @param	PhysicsAsset		설정할 PhysicsAsset 인스턴스
     * @param	SkelMesh			물리 에셋을 생성할 스켈레탈 메시
     */
    bool CreateFromSkeletalMesh(UPhysicsAsset* PhysicsAsset, USkeletalMesh* SkelMesh);

    int32 CreateNewConstraint(UPhysicsAsset* PhysAsset, FName InConstraintName, UPhysicsConstraintTemplate* InConstraintSetup, FName ParentBone, FName ChildBone);
    void DestroyConstraint(UPhysicsAsset* PhysAsset, int32 ConstraintIndex);

    int32 CreateNewBody(UPhysicsAsset* PhysAsset, FName InBoneName);

}
