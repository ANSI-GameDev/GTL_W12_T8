#pragma once
#include "ShapeElem.h"
#include "Math/Transform.h"
#include "Math/Vector.h"

struct FKSphereElem : public FKShapeElem
{
    FKSphereElem()
        : FKShapeElem(EAggCollisionShape::Sphere)
        , Radius(0.0f)
        , Center(FVector::ZeroVector)
    {
    }

    virtual ~FKSphereElem();

    friend bool operator==(const FKSphereElem& LHS, const FKSphereElem& RHS)
    {
        return (LHS.Center == RHS.Center && LHS.Radius == RHS.Radius);
    }

    FTransform GetTransform() const
    {
        return FTransform(Center);
    };

    void SetTransform(const FTransform& InTransform)
    {
        Center = InTransform.GetTranslation();
    }

public:

    FVector Center;
    float Radius;
};
