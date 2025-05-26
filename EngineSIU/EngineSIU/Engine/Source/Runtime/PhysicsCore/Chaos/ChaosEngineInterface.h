#pragma once


enum ELinearConstraintMotion
{
    LCM_Free,       // 이동이 자유로운 경우    
    LCM_Limited,    // 이동 허용하되 Limit 안에서만
    LCM_Locked,     // 이동 불가
    LCM_MAX
};

enum EAngularConstraintMotion
{
    /* Constraint 없이 회전이 자유로운 경우 */
    ACM_Free,
    /* 제한적인 자유도가 위 축에 있는 경우 */
    ACM_Limited,
    /* Fully Constraint against this axis */
    ACM_Locked,
    ACM_MAX
};

/* Frame1과 Frame2 : 각각 자식 본과 부모 본의 Constraint 좌표계 */
namespace EConstraintFrame
{
    enum Type : int
    {
        Frame1,
        Frame2
    };
}
