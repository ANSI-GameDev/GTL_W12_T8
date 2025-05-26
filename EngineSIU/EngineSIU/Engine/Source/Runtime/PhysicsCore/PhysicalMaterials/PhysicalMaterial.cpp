#include "PhysicalMaterial.h"

UPhysicalMaterial::UPhysicalMaterial()
    : Friction(0.5f)
    , StaticFriction(0.5f)
    , Restitution(0.6f)
    //, Density(1.0f)
{
}

UPhysicalMaterial::~UPhysicalMaterial()
{

}
