#pragma once
#include "PrimitiveElem.h"
#include "Container/Array.h"

struct FKAggregateGeom
{
    TArray<FKSphereElem> SphereElems;

    TArray<FKBoxElem> BoxElems;

    TArray<FKSphylElem> CapsuleElems;
};
