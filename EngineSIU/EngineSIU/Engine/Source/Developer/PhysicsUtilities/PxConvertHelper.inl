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
inline PxQuat FQuat::ToPxQuat() const 
{
    FQuat tmp = *this;
    tmp.Normalize();
    PxQuat quat = { tmp.X, tmp.Y, tmp.Z, tmp.W };
    return quat;
}
inline PxQuat FQuat::ToPxQuat()
{
    this->Normalize();
    PxQuat quat = { this->X, this->Y, this->Z, this->W };
    return quat;
}



