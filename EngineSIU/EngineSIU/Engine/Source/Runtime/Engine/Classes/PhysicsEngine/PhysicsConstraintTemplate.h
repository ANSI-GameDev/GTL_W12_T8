#pragma once
#include "ConstraintInstance.h"
#include "UObject/ObjectMacros.h"

/* Constraint의 "설계도"
 * Editor / Asset 설정용 객체, 애셋 내 영구 저장되는 수명을 가짐
 */
class UPhysicsConstraintTemplate : public UObject
{
    DECLARE_CLASS(UPhysicsConstraintTemplate, UObject)
public:
    UPhysicsConstraintTemplate() = default;
    void SetDefaultProfile(FConstraintInstance& CI);
    virtual void Serialize(FArchive& Ar) override;



    FConstraintInstance DefaultInstance;
private:
    /* 아무 Profile이 선택되지 않았을 때에만 사용, DefaultInstance 저장시에만 Serialize용으로 사용 */
    FConstraintProfileProperties DefaultProfile;
};
