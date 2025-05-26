#pragma once
#include "BodyInstance.h"
#include "ConstraintTypes.h"
#include "PhysScene.h"
#include "Components/SkeletalMeshComponent.h"
#include "HAL/PlatformType.h"
#include "Math/Vector.h"
#include "UObject/NameTypes.h"

enum class EConstraintTransformComponentFlags : uint8
{
    None = 0,

    ChildPosition = 1 << 0,
    ChildRotation = 1 << 1,
    ParentPosition = 1 << 2,
    ParentRotation = 1 << 3,

    AllChild = ChildPosition | ChildRotation,
    AllParent = ParentPosition | ParentRotation,
    AllPosition = ChildPosition | ParentPosition,
    AllRotation = ChildRotation | ParentRotation,
    All = AllChild | AllParent
};

struct FConstraintProfileProperties
{
    FConstraintProfileProperties();

    /* R^3 상에서 거리 벡터 크기 제한 = 탈골 방지
     * 두 rigid body 사이의 기준점들이 서로 너무 멀어지지 않도록 제한
     */
    FLinearConstraint LinearLimit;

    FConeConstraint ConeLimit;

    FTwistConstraint TwistLimit;

    
    uint8 bDisableCollision : 1;        /* Constraint로 연결된 Bodies Collision 비활성화 여부 */
    uint8 bParentDominates : 1;         /* Child Motion에 Parent Body가 영향받을지 여부 */

    // [Projection] 기타 회전 오차, 선형 위치 오차 보정 - 작은 오차는 물리 보정, 큰 오차는 순간 이동으로 보정함
    // [Drive (스프링) ] 선형 / 회전 목표로 끌어당기는 제약
};

struct FConstraintInstanceBase
{
    FConstraintInstanceBase();
    void Reset();
    FPhysScene* GetPhysScene() const;
    const FPhysScene* GetPhysicsScene() const;

    /* Skeletal Mesh Component 배열 내의 인덱스 */
    int32 ConstraintIndex;

    FPhysScene* PhysScene;
};

/* 실제 시뮬레이션에 사용되는 물리 엔진 레벨의 Constraint 데이터
 * 런타임에 UPhysicsConstraintTemplate의 DefaultInstance로부터 복사되어 생성
 */
struct FConstraintInstance : public FConstraintInstanceBase
{
public:
    FConstraintInstance();
    void CopyConstraintParamsFrom(const FConstraintInstance* FromInstance);
    void SetRefPosition(EConstraintFrame::Type Frame, const FVector& Position);
    void SetRefOrientation(EConstraintFrame::Type Frame, const FVector& InPriAxis, const FVector& InSecAxis);


    const FName& GetChildBoneName() const;
    const FName& GetParentBoneName() const;

    float GetLinearLimit() const;
    void UpdateLinearLimit();
    void UpdateAngularLimit();
    void InitConstraint(FBodyInstance* Body1, FBodyInstance* Body2, float Scale, USkeletalMeshComponent* OwningComponent);
    FTransform CalculateDefaultChildTransform() const;
    FTransform CalculateDefaultParentTransform(const UPhysicsAsset* PhysicsAsset) const;
    void SnapTransformsToDefault(const EConstraintTransformComponentFlags SnapFlags, const UPhysicsAsset* const PhysicsAsset);


    FName JointName;
    FName ConstraintBone1;
    FName ConstraintBone2;

    
private:
    float LastKnownScale; // 초기화 당시 컴포넌트의 Scale

public:

    FConstraintProfileProperties ProfileInstance;

    /* Reference Frame 기준 Body1 Constraint 위치 */
    FVector Pos1;

    /* Reference Frame 기준 Body1 Primary (twist) Axis*/
    FVector PriAxis1;

    /* Reference Frame 기준 Body1 Secondary Axis, 위와 직교함 */
    FVector SecAxis1;

    /* 이하 동일, Body2의 경우 주로 Skeletal Mesh의 Parent Body */
    FVector Pos2;
    FVector PriAxis2;
    FVector SecAxis2;

    /* Bone의 Local X가 아닌 방향으로 twist를 제한하거나 */
    FRotator AngularRotationOffset;
    uint32 bScaleLinearLimits : 1;

    float AverageMass;
};
