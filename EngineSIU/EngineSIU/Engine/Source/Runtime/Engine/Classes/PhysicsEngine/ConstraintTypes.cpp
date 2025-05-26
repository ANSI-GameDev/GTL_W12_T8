#include "ConstraintTypes.h"

FConstraintBaseParams::FConstraintBaseParams()
    : Stiffness(50.f)
    , Damping(5.f)
    , Restitution(0.f)
    , ContactDistance(1.f)
    , bSoftConstraint(false)
{

}

FLinearConstraint::FLinearConstraint()
    : Limit(0.f)
    , XMotion(LCM_Locked)
    , YMotion(LCM_Locked)
    , ZMotion(LCM_Locked)
{
    ContactDistance = 5.f;
    Stiffness = 0.f;
    Damping = 0.f;
}

FConeConstraint::FConeConstraint()
    : Swing1LimitDegrees(45.f)
    , Swing2LimitDegrees(45.f)
    , Swing1Motion(ACM_Free)
    , Swing2Motion(ACM_Free)
{
    bSoftConstraint = true;
    ContactDistance = 1.f;
}

FTwistConstraint::FTwistConstraint()
    : TwistLimitDegrees(45)
    , TwistMotion(ACM_Free)
{
    bSoftConstraint = true;
    ContactDistance = 1.f;
}




