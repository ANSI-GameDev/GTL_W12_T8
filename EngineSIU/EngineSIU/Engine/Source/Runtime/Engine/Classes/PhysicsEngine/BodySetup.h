#pragma once
#include "AggregateGeom.h"
#include "PhysicsCore/Public/BodySetupCore.h"
#include "UObject/ObjectMacros.h"

class UBodySetup : public UBodySetupCore
{
    DECLARE_CLASS(UBodySetup, UBodySetupCore)

    UBodySetup() = default;
    
    FKAggregateGeom AggGeom;
public:
    
};
