#pragma once
#include "ShapeElem.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"
#include "Math/Vector.h"

struct FKSphylElem : public FKShapeElem
{

    FKSphylElem()
        : FKShapeElem(EAggCollisionShape::Sphyl)
        , Center(FVector::ZeroVector)
        , Rotation(FRotator::ZeroRotator)
        , Radius(1), Length(1)
    {

    }

    friend bool operator==(const FKSphylElem& LHS, const FKSphylElem& RHS)
    {
        return (LHS.Center == RHS.Center &&
            LHS.Rotation == RHS.Rotation &&
            LHS.Radius == RHS.Radius &&
            LHS.Length == RHS.Length);
    };


    FTransform GetTransform() const
    {
        return FTransform(Rotation, Center);
    };

    void SetTransform(const FTransform& InTransform)
    {
        Rotation = InTransform.Rotator();
        Center = InTransform.GetTranslation();
    }

    FVector Center;
    FRotator Rotation;
    float Radius;
    float Length;
};
