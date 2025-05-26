#pragma once
#include "ShapeElem.h"

struct FKBoxElem : public FKShapeElem
{
    FKBoxElem()
    : FKShapeElem(EAggCollisionShape::Box)
    , Center( FVector::ZeroVector )
    , Rotation( FRotator::ZeroRotator )
    , Extent(FVector::OneVector)
    {
    }
    
    FKBoxElem( float s )
    : FKShapeElem(EAggCollisionShape::Box)
    , Center( FVector::ZeroVector )
    , Rotation(FRotator::ZeroRotator)
    , Extent(FVector(s,s,s))
    {
    }
    
    FKBoxElem( FVector Extent ) 
    : FKShapeElem(EAggCollisionShape::Box)
    , Center( FVector::ZeroVector )
    , Rotation(FRotator::ZeroRotator)
    , Extent(Extent)
    {
    }

    friend bool operator==( const FKBoxElem& LHS, const FKBoxElem& RHS )
    {
        return ( LHS.Center == RHS.Center &&
            LHS.Rotation == RHS.Rotation &&
            LHS.Extent.X == RHS.Extent.X &&
            LHS.Extent.Y == RHS.Extent.Y &&
            LHS.Extent.Z == RHS.Extent.Z );
    }

    FTransform GetTransform() const
    {
        return FTransform(Rotation, Center);
    }

    void SetTransform(const FTransform& InTransform)
    {
        Rotation = InTransform.Rotator();
        Center = InTransform.GetTranslation();
    }
    
    FVector Center;
    FRotator Rotation;
    FVector Extent;
};
