#pragma once
#include <foundation/PxTransform.h>
#include <foundation/PxVec3.h>

#include "Math/Transform.h"
#include "Math/Vector.h"

using namespace physx;

inline FVector::FVector(const PxVec3& InVec) : X(InVec.x), Y(InVec.y), Z(InVec.z){}
inline PxVec3 FVector::ToPxVec3() const
{
    return {this->X, this->Y, this->Z};
}

inline FTransform::FTransform(const physx::PxTransform& InTransform) : Translation(InTransform.p), Rotation(InTransform.q), Scale3D(FVector::OneVector) {}
inline PxTransform FTransform::ToPxTransform()
{
    return {this->Translation.ToPxVec3(), this->Rotation.ToPxQuat()};
}

inline FQuat::FQuat(const physx::PxQuat& InQuat):X(InQuat.x), Y(InQuat.y), Z(InQuat.z), W(InQuat.w){}
inline PxQuat FQuat::ToPxQuat()
{
    return {this->X, this->Y, this->Z, this->W};
}
FVector FQuat::GetUnitAxis(EAxis::Type Axis) const
{
    switch (Axis)
    {
    case EAxis::X:
        return RotateVector(FVector::XAxisVector);
    case EAxis::Y:
        return RotateVector(FVector::YAxisVector);
    case EAxis::Z:
        return RotateVector(FVector::ZAxisVector);
    default:
        return FVector::ZeroVector;
    }
}
