#pragma once
#include "ShapeElem.h"
#include "Math/Quat.h"

struct FKBoxElem : public FKShapeElem
{
    FKBoxElem()
        : FKShapeElem(EAggCollisionShape::Box)
        , Orientation_DEPRECATED(FQuat::Identity)
        , Center(FVector::ZeroVector)
        , Rotation(FRotator::ZeroRotator)
        , X(1), Y(1), Z(1)
    {

    }

    FTransform GetTransform() const
    {
        return FTransform(Rotation, Center);
    };


    void SetTransform(const FTransform& InTransform)
    {
        Rotation = InTransform.Rotator();
        Center = InTransform.GetTranslation();
    }

    FQuat Orientation_DEPRECATED;

    FVector Center;
    FRotator Rotation;

    /* Box Extent - axis aligned */
    float X;
    float Y;
    float Z;
};
