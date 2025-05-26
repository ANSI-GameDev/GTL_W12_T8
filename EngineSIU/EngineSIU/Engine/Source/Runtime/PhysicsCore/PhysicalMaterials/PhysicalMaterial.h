#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UPhysicalMaterial : public UObject
{
    DECLARE_CLASS(UPhysicalMaterial, UObject)
public:
    UPhysicalMaterial();
    virtual ~UPhysicalMaterial();


    float Friction;
    float StaticFriction;
    float Restitution;

    float Density;
};

