#pragma once
#include "Engine/EngineTypes.h"
#include "UObject/NameTypes.h"

namespace EAggCollisionShape
{
    enum Type : int
    {
        Sphere,
        Box,
        Sphyl,
        Convex,
        TaperedCapsule,
        LevelSet,
        SkinnedLevelSet,
        Unknown
    };
}

struct FKShapeElem
{
    FKShapeElem()
        : RestOffset(0.f)
        , bIsGenerated(false)
        , ShapeType(EAggCollisionShape::Unknown)
        , bContributeToMass(true)
        , CollisionEnabled(ECollisionEnabled::QueryAndPhysics)
    {
    }

    FKShapeElem(EAggCollisionShape::Type InShapeType)
        : RestOffset(0.f)
        , bIsGenerated(false)
        , ShapeType(InShapeType)
        , bContributeToMass(true)
        , CollisionEnabled(ECollisionEnabled::QueryAndPhysics)
    {
    }

    virtual ~FKShapeElem();

    float RestOffset;

    uint8 bIsGenerated : 1;

    FName Name;

    EAggCollisionShape::Type ShapeType;

    uint8 bContributeToMass : 1;

    ECollisionEnabled::Type CollisionEnabled;



};
