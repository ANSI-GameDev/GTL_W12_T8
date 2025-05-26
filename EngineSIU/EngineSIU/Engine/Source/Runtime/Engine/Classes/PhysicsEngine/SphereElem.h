#pragma once
#include "ShapeElem.h"
#include "Math/Transform.h"
#include "Math/Vector.h"

struct FKSphereElem : public FKShapeElem
{
    FKSphereElem() 
    : FKShapeElem(EAggCollisionShape::Sphere)
    , Center( FVector::ZeroVector )
    , Radius(1){}

    FKSphereElem(float InRadius)
        : FKShapeElem(EAggCollisionShape::Sphere)
    ,Center(FVector::ZeroVector)
    ,Radius(InRadius){}

    ~FKSphereElem();
    
    FVector Center;
    float Radius;

    friend bool operator==( const FKSphereElem& LHS, const FKSphereElem& RHS )
    {
        return ( LHS.Center == RHS.Center &&
            LHS.Radius == RHS.Radius );
    }

    FTransform GetTransform() const
    {
        return FTransform( Center );
    }

    void SetTransform(const FTransform& InTransform)
    {
        Center = InTransform.GetTranslation();
    }
    
    // FBoundingBox CalcAABB(const FTransform& BoneTM, float Scale) const;
    // float GetShortestDistanceToPoint(const FVector& WorldPosition, const FTransform& BodyToWorldTM) const;
    // float GetClosestPointAndNormal(const FVector& WorldPosition, const FTransform& BodyToWorldTM, FVector& ClosestWorldPosition, FVector& Normal) const;
};
