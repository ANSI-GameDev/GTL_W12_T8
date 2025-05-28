#pragma once
#include "BonePose.h"

class UAnimInstance;

struct FAnimationBaseContext
{
    UAnimInstance* AnimInstance;

    FAnimationBaseContext(UAnimInstance* InAnimInstance)
        : AnimInstance(InAnimInstance)
    {}
};

/** Evaluation context passed around during animation tree evaluation */
struct FPoseContext : public FAnimationBaseContext
{
    FCompactPose Pose;
    FCompactPose GlobalPose;
    // FBlendedCurve Curve;

    FPoseContext(UAnimInstance* InAnimInstance)
        : FAnimationBaseContext(InAnimInstance)
    {}
};
