#pragma once
#include "Define.h"
#include "Math/Color.h"
#include "Math/Transform.h"
#include "UObject/NameTypes.h"

struct FTransform;

namespace EAggCollisionShape
{
enum Type : int
{
    Sphere,
    Box,
    Unknown
};
}

struct FKShapeElem
{
    FKShapeElem(EAggCollisionShape::Type InShapeType)
        :ShapeType(InShapeType)
    {}

    FName Name;

    EAggCollisionShape::Type ShapeType;
    
    /** Get the user-defined name for this shape */
    const FName& GetName() const { return Name; }

    /** Set the user-defined name for this shape */
    void SetName(const FName& InName) { Name = InName; }
};

struct FKSphereElem : public FKShapeElem
{
    FKSphereElem() 
    : FKShapeElem(EAggCollisionShape::Sphere)
    , Center( FVector::ZeroVector )
, Radius(1){}
    
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
    
    FBoundingBox CalcAABB(const FTransform& BoneTM, float Scale) const;

    /**	
 * Finds the shortest distance between the element and a world position. Input and output are given in world space
 * @param	WorldPosition	The point we are trying to get close to
 * @param	BodyToWorldTM	The transform to convert BodySetup into world space
 * @return					The distance between WorldPosition and the shape. 0 indicates WorldPosition is inside the shape.
 */
    float GetShortestDistanceToPoint(const FVector& WorldPosition, const FTransform& BodyToWorldTM) const;

    /**	
     * Finds the closest point on the shape given a world position. Input and output are given in world space
     * @param	WorldPosition			The point we are trying to get close to
     * @param	BodyToWorldTM			The transform to convert BodySetup into world space
     * @param	ClosestWorldPosition	The closest point on the shape in world space
     * @param	Normal					The normal of the feature associated with ClosestWorldPosition.
     * @return							The distance between WorldPosition and the shape. 0 indicates WorldPosition is inside the shape.
     */
    float GetClosestPointAndNormal(const FVector& WorldPosition, const FTransform& BodyToWorldTM, FVector& ClosestWorldPosition, FVector& Normal) const;
};

struct FKBoxElem : public FKShapeElem
{
    FKBoxElem()
    : FKShapeElem(EAggCollisionShape::Box)
    , Center( FVector::ZeroVector )
    , Rotation( FRotator::ZeroRotator )
    , X(1), Y(1), Z(1)
    {
    }
    
    FKBoxElem( float s )
    : FKShapeElem(EAggCollisionShape::Box)
    , Center( FVector::ZeroVector )
    , Rotation(FRotator::ZeroRotator)
    , X(s), Y(s), Z(s)
    {
    }
    
    FKBoxElem( float InX, float InY, float InZ ) 
    : FKShapeElem(EAggCollisionShape::Box)
    , Center( FVector::ZeroVector )
    , Rotation(FRotator::ZeroRotator)
    , X(InX), Y(InY), Z(InZ)
    {
    }

    friend bool operator==( const FKBoxElem& LHS, const FKBoxElem& RHS )
    {
        return ( LHS.Center == RHS.Center &&
            LHS.Rotation == RHS.Rotation &&
            LHS.X == RHS.X &&
            LHS.Y == RHS.Y &&
            LHS.Z == RHS.Z );
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
    float X;
    float Y;
    float Z;

};

#if 0 
struct FKCapsuleElem: public FKShapeElem
{
    
};
#endif
