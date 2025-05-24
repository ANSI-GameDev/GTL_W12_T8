#include "ConstraintInstance.h"

FConstraintInstanceBase::FConstraintInstanceBase()
{
    Reset();
}

void FConstraintInstanceBase::Reset()
{
    ConstraintIndex = 0;
}

/** Constructor **/
FConstraintInstance::FConstraintInstance()
	: FConstraintInstanceBase()
	, AngularRotationOffset(FRotator::ZeroRotator)
	, AverageMass(0.f)
{
	ChildPos = FVector(0.0f, 0.0f, 0.0f);
	ChildPriAxis = FVector(1.0f, 0.0f, 0.0f);
	ChildSecAxis = FVector(0.0f, 1.0f, 0.0f);

	ParentPos = FVector(0.0f, 0.0f, 0.0f);
	ParentPriAxis = FVector(1.0f, 0.0f, 0.0f);
	ParentSecAxis = FVector(0.0f, 1.0f, 0.0f);
}
